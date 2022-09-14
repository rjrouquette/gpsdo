//
// Created by robert on 9/13/22.
//

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <sysexits.h>

#include "flexfis.h"
#include "qnorm.h"

class CsvRow {
        public:
        explicit CsvRow(const char *text, bool collapse_empty = false);
        CsvRow(const char *text, size_t len, bool collapse_empty = false);
        virtual ~CsvRow();

        inline size_t count() const { return cnt; }
        inline const char* field(size_t index) const { return fields[index]; }
        inline const char* operator[](size_t index) const { return fields[index]; }

        private:
        char *data;
        const char **fields;
        size_t cnt;
};

struct FlexNode {
    int touched;
    float n;
    float center[DIM_INPUT+1];
    float cov[((DIM_INPUT+2)*(DIM_INPUT+1))/2];
    float reg[DIM_INPUT];
};


int cols, rows, tcnt;
float *data = nullptr;

extern volatile int nodeCount;
extern struct QNorm norms[DIM_INPUT+1];
extern struct FlexNode nodes[MAX_NODES];

float parseFloat(const char *str);
bool loadData(const char *fname);

void printBins() {
    for(int i = 0; i < QNORM_SIZE; i++) {
        for(int j = 0; j <= DIM_INPUT; j++) {
            std::cout << " " << norms[j].bins[i].mean;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printNodes() {
    std::cout << nodeCount << std::endl;
    for(const auto &node : nodes) {
        std::cout << " " << node.touched;
        std::cout << " " << node.n;
        std::cout << std::endl;
        for(auto &v : node.center)
            std::cout << " " << v;
        std::cout << std::endl;
        for(auto &v : node.cov)
            std::cout << " " << v;
        std::cout << std::endl;
        for(auto &v : node.reg)
            std::cout << " " << v;
        std::cout << std::endl;
        std::cout << std::endl;
    }
}

int main(int argc, char **argv) {
    if(!loadData(argv[1])) return EX_IOERR;
    tcnt = (1 * rows) / 4;
    tcnt = 1024;
    //rows = 65536;

    std::cout << "sizeof(QNorm): " << sizeof(QNorm) << std::endl;
    std::cout << "sizeof(FlexNode): " << sizeof(FlexNode) << std::endl;
    std::cout << "sizeof(norms): " << sizeof(norms) << std::endl;
    std::cout << "sizeof(nodes): " << sizeof(nodes) << std::endl;

    std::cout << "loaded csv data" << std::endl;
    flexfis_init(tcnt, ::data, cols, ::data + cols - 1, cols);
//    flexfis.plotClusters(1, 0, 1024, "flexfis.nodes.train.png");
//    flexfis.plotNorms("flexfis.norms.train");
    std::cout << "flexfis initialized" << std::endl;

//    printBins();
//    printNodes();

    auto fout = fopen("dist/flexfis.test.csv", "w");
    fprintf(fout, "temperature,compensation,prediction,error\n");
    double corr = 0, rmsA = 0, rmsB = 0;
    double rmse = 0;
    auto t = ::data + (cols * tcnt);
    auto r = ::data + (cols * tcnt);
    for(int i = tcnt; i < rows; i++) {
        if(!std::isfinite(r[0])) {
            r += cols;
            t += cols;
            continue;
        }
        auto a = flexfis_predict(r);
        if(i >= rows - 7200) {
            fprintf(fout, "%f,%f,%f,%f\n", r[0], r[cols-1], a, a - r[cols-1]);
        }
        flexfis_update(t, t[cols-1]);
        t += cols;
        auto b = r[cols - 1];
        auto diff = a - b;
        rmse += diff * diff;
        corr += a * b;
        rmsA += a * a;
        rmsB += b * b;
        r += cols;
    }
    fclose(fout);
    corr /= sqrt(rmsA) * sqrt(rmsB);
    rmse = sqrt(rmse / (rows - tcnt));
//    flexfis.plotClusters(1, 0, 1024, "flexfis.nodes.evolv.png");
//    flexfis.plotNorms("flexfis.norms.evolv");

    std::cout << std::setprecision(9);
    std::cout << std::endl << "corr: " << corr;
    std::cout << std::endl << "rmse: " << rmse << std::endl;

//    printBins();
//    printNodes();

    delete[] data;
    return 0;
}

bool loadData(const char *fname) {
    std::string line;
    std::ifstream fin(fname);
    if(!fin.good()) return false;

    cols = 0;
    rows = 0;
    getline(fin, line);
    while(fin.good()) {
        if(!line.empty()) {
            CsvRow row(line.c_str());
            if(cols == 0 && row.count() > 1)
                cols = (int)row.count();
            if(cols == (int)row.count())
                ++rows;
        }
        getline(fin, line);
    }
    fin.close();
    fin.clear();
    fin.open(fname);

    float delay[DIM_INPUT] = { 0 };
    ::data = new float[rows * (cols+DIM_INPUT-3)];
    bzero(::data, rows * (cols+DIM_INPUT-3) * sizeof(float));
    auto ptr = ::data;
    rows = 0;
    getline(fin, line);
    while(fin.good()) {
        if(!line.empty()) {
            CsvRow row(line.c_str());
            if(cols == (int)row.count()) {
                for(int i = DIM_INPUT-1; i > 0; i--)
                    delay[i] = delay[i-1];
                delay[0] = parseFloat(row[2]);
                for(auto v : delay)
                *(ptr++) = v;
                for(int i = 3; i < cols; i++)
                    *(ptr++) = parseFloat(row[i]);

                ptr -= cols+DIM_INPUT-3;
                bool ok = true;
                for(int i = 0; i < cols+DIM_INPUT-3; i++) {
                    if(!std::isfinite(ptr[i])) ok = false;
                }
                if(ok) {
                    ++rows;
                    ptr += cols+DIM_INPUT-3;
                }
            }
        }
        getline(fin, line);
    }
    fin.close();
    cols += DIM_INPUT - 3;

    return true;
}

CsvRow::CsvRow(const char *text, bool collapse_empty) :
CsvRow(text, strlen(text), collapse_empty)
{

}

CsvRow::CsvRow(const char *text, size_t len, bool collapse_empty) {
    if(!len) {
        data = nullptr;
        fields = nullptr;
        cnt = 0;
        return;
    }

    data = new char[len+1];
    memcpy(data, text, len);
    data[len] = 0;

    // Locate delimiters.
    size_t n = 0;
    bool delim = true;
    char quoted = 0;
    for(size_t i = 0; i < len; i++) {
        char c = data[i];
        if(c == '\'' || c == '"') {
            if(!quoted) {
                quoted = c;
            }
            else if(quoted == c && data[i-1] != '\\') {
                quoted = 0;
            }
            delim = false;
        }
        else if(!quoted) {
            if(c == ',' || c == '\t') {
                data[i] = 0;
                if(!(collapse_empty && delim)) {
                    n++;
                }
                delim = true;
            }
            else {
                delim = false;
            }
        }
    }

    cnt = n + 1;
    fields = new const char*[cnt];

    n = 0;
    delim = true;
    for(size_t i = 0; i < len; i++) {
        if(data[i]) {
            if(delim) {
                fields[n++] = &(data[i]);
            }
            delim = false;
        }
        else {
            if(!(collapse_empty && delim)) {
                if(delim) {
                    fields[n++] = &(data[i]);
                }
            }
            delim = true;
        }
    }

    if(delim && n < cnt) {
        fields[n] = &(data[len]);
    }

    if(collapse_empty && !fields[cnt-1][0]) {
        cnt--;
    }
}

CsvRow::~CsvRow() {
    delete[] data;
    delete[] fields;
}

float parseFloat(const char *str) {
    auto len = strlen(str);
    if(len < 1)
        return NAN;
    char *endptr = nullptr;
    auto value = strtod(str, &endptr);
    if(endptr == str + len)
        return (float) value;
    else
        return NAN;
}
