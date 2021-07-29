
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <unistd.h>

#define IPPM_PREC (0.000026f)

using namespace std;

struct Sample {
    float temp;
    float ppm;
    float error;

    Sample () {
        temp = NAN;
        ppm = NAN;
        error = NAN;
    }

    void clear() {
        temp = NAN;
        ppm = NAN;
        error = NAN;
    }

    bool isValid() {
        return isfinite(temp) && isfinite(ppm) && isfinite(error);
    }
};

static void accumulate(const int32_t value, int64_t &acc) {
    acc += (((int64_t)value) << 16) - (acc >> 16);
}

static void accumulate(const double value, double &acc) {
    acc += (value - acc) / 65536.0;
}

struct FixedMath {
    uint64_t count[256];
    int64_t n[256];
    int64_t mat[256][4];

    FixedMath() : count{}, mat{} {
        bzero(count, sizeof(count));
        bzero(mat, sizeof(mat));
    }

    void update(const Sample &s) {
        int8_t ipart = (int) s.temp;
        int32_t fpart = floorf((s.temp - ipart) * 256.0f) - 128;
        uint8_t tidx = ipart;
        int32_t ippm = floorf(s.ppm / IPPM_PREC);

        auto &m = mat[tidx];
        accumulate((fpart * fpart) << 16, m[0]);
        accumulate(fpart << 8, m[1]);
        accumulate(ippm * fpart, m[2]);
        accumulate(ippm, m[3]);

        accumulate(1, n[tidx]);
        ++count[tidx];
    }

    bool coeff(const float temp, int32_t &m, int32_t &b) {
        int8_t ipart = (int) temp;
        uint8_t tidx = ipart;
        if(n[tidx] == 0) return false;

        auto &mt = mat[tidx];
        int64_t A = mt[0] / n[tidx]; // 0.32
        int64_t B = mt[1] / n[tidx]; // 16.16
        int64_t C = mt[2] / n[tidx]; // 24.8
        int64_t D = mt[3] / n[tidx]; // 32.0
        C <<= 8; // 24.16

        uint32_t Z = A - (B * B);
        if(Z == 0 || count[tidx] < 1024) {
            m = 0;
            b = D;
        } else {
            m = (((C * 1) - (B * D)) << 24) / Z;
            b = (((A * D) - (B * C))) / Z;
        }
        return true;
    }
} fixedMath;

struct FloatMath {
    uint64_t count[256];
    double n[256];
    double mat[256][4];

    FloatMath() : count{}, mat{} {
        bzero(count, sizeof(count));
        bzero(mat, sizeof(mat));
    }

    void update(const Sample &s) {
        int8_t ipart = (int) s.temp;
        double fpart  = (s.temp - ipart) - 0.5;
        uint8_t tidx = ipart;

        auto &m = mat[tidx];
        accumulate(fpart * fpart, m[0]);
        accumulate(fpart, m[1]);
        accumulate(fpart * s.ppm, m[2]);
        accumulate(s.ppm, m[3]);

        accumulate(1.0f, n[tidx]);
        ++count[tidx];
    }

    bool coeff(const float temp, double &m, double &b) {
        uint8_t tidx = (uint8_t) ((int) temp);
        if(n[tidx] == 0) return false;

        auto &mt = mat[tidx];
        double A = mt[0] / n[tidx];
        double B = mt[1] / n[tidx];
        double C = mt[2] / n[tidx];
        double D = mt[3] / n[tidx];

        const double Z = A - (B * B);
        if(Z == 0 || count[tidx] < 1024) {
            m = 0;
            b = D;
        } else {
            m = ((C * 1) - (B * D)) / Z;
            b = ((A * D) - (B * C)) / Z;
        }
        return true;
    }
} floatMath;



/* File format:
name: gpsdo
time                temp               ppm                  error
----                ----               ---                  -----
1596026531000000000 52.4               -0.7289              11.2
1596026542000000000 52.4               -0.729               10
1596026553000000000 52.2               -0.7289              10
1596026564000000000 52.3               -0.7289              10
...
*/

int main(int argc, char **argv) {
    ifstream fin(argv[1]);

    // skip over headers
    string line, discard;
    for(int i = 0; i < 3; i++)
        getline(fin, discard);

    // read data
    list<Sample> data;
    Sample row;
    getline(fin, line);
    while(fin.good()) {
        istringstream parser(line);
        row.clear();
        parser >> discard;
        parser >> row.temp;
        parser >> row.ppm;
        parser >> row.error;
        if(row.isValid())
            data.emplace_back(row);
        
        getline(fin, line);
    }
    fin.close();
    cout << fixed << setprecision(6);
    cout << "loaded " << data.size() << " data samples" << endl;
    cout << endl;

    double maxI = 0, maxF = 0;
    double rmseI = 0, rmseF = 0;
    size_t cntI = 0, cntF = 0;
    size_t errI = 0, errF = 0;
    for(auto s : data) {
        if(s.error > 250.0f) continue;

        int32_t im, ib;
        if(fixedMath.coeff(s.temp, im, ib)) {
            int8_t it = (int) s.temp;
            int32_t ix = floorf((s.temp - it) * 256.0f) - 128;
            auto iy = ((im * ix) >> 16) + ib;
            double id = (iy * IPPM_PREC) - s.ppm;
            if(std::isfinite(id)) {
                rmseI += id * id;
                ++cntI;
            }
            maxI = std::max(maxI, std::abs(id));
            if(std::abs(id) > 0.100) {
                cout << s.temp << " " << s.ppm << " " << iy << endl;
                ++errI;
            }
        }
        fixedMath.update(s);

        double fm, fb;
        if(floatMath.coeff(s.temp, fm, fb)) {
            auto fx = (s.temp - ((int)s.temp)) - 0.5;
            auto fy = (fm * fx) + fb;
            auto fd = fy - s.ppm;
            if(std::isfinite(fd)) {
                rmseF += fd * fd;
                ++cntF;
            }
            maxF = std::max(maxF, std::abs(fd));
            if(std::abs(fd) > 0.100) {
                cout << s.temp << " " << s.ppm << " " << fy << endl;
                ++errF;
            }
        }
        floatMath.update(s);
    }
    cout << "rmseI: " << sqrt(rmseI / cntI) << " ppm (" << maxI << " ppm) [" << errI << "]" << endl;
    cout << "rmseF: " << sqrt(rmseF / cntF) << " ppm (" << maxF << " ppm) [" << errF << "]" << endl;
    cout << endl;

    cout << "fixed point math:" << endl;
    cout << "Temp.";
    cout << setw(8) << "Count";
    cout << setw(12) << "Norm";
    cout << setw(12) << "X * X";
    cout << setw(12) << "X * 1";
    cout << setw(12) << "Y * X";
    cout << setw(12) << "Y * 1";
    cout << setw(12) << "m";
    cout << setw(12) << "b";
    cout << endl;

    for(int i = 0; i < 256; i++) {
        if(fixedMath.count[i] == 0) continue;
        cout << setw(3) << i << ": ";
        cout << setw(8) << fixedMath.count[i];
        const auto &mt = fixedMath.mat[i];
        cout << setw(12) << fixedMath.n[i] / 65536.0f / 65536.0f;
        cout << setw(12) << ((float) mt[0] / (float) fixedMath.n[i]) / 65536.0f / 65536.0f;
        cout << setw(12) << ((float) mt[1] / (float) fixedMath.n[i]) / 65536.0f;
        cout << setw(12) << ((float) mt[2] / (float) fixedMath.n[i]) * IPPM_PREC / 256.0f;
        cout << setw(12) << ((float) mt[3] / (float) fixedMath.n[i]) * IPPM_PREC;

        int32_t m, b;
        fixedMath.coeff(i, m, b);
        cout << setw(12) << (m * IPPM_PREC / 256.0f);
        cout << setw(12) << (b * IPPM_PREC);
        cout << endl;
    }
    cout << endl;

    cout << "floating point math:" << endl;
    cout << "Temp.";
    cout << setw(8) << "Count";
    cout << setw(12) << "Norm";
    cout << setw(12) << "X * X";
    cout << setw(12) << "X * 1";
    cout << setw(12) << "Y * X";
    cout << setw(12) << "Y * 1";
    cout << setw(12) << "m";
    cout << setw(12) << "b";
    cout << endl;

    for(int i = 0; i < 256; i++) {
        if(fixedMath.count[i] == 0) continue;
        cout << setw(3) << i << ": ";
        cout << setw(8) << floatMath.count[i];
        cout << setw(12) << floatMath.n[i];
        for(auto m : floatMath.mat[i])
            cout << setw(12) << (m / floatMath.n[i]);
        double m, b;
        floatMath.coeff(i, m, b);
        cout << setw(12) << m;
        cout << setw(12) << b;
        cout << endl;
    }
    cout << endl;

    return 0;
}
