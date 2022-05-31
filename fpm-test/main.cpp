
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

#define TEMP_SPAN (8)
#define TEMP_PREC (0.125f)
#define BINS (256)
#define IPPM_PREC (0.0001164f)

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

// A = A + ((B - A) / 2^16)
static void accumulate(int64_t &acc, const int64_t value) {
    acc -= acc >> 12u;
    acc += value << 20u;
}

union u32 {
    uint16_t word[2];
    uint32_t full;
};

union i64 {
    uint16_t word[4];
    int64_t full;
};

// signed/unsigned integer long-division
int32_t div64s32u(int64_t rem, uint32_t div) {
    // sign detection
    const uint8_t neg = rem < 0;
    if(neg) rem = -rem;

    // unsigned long division
    uint8_t carry = 0;
    uint32_t head = 0;
    uint32_t quo = 0;
    for(int8_t shift = 63; shift >= 0; --shift) {
        // shift quotient to make room for new bit
        quo <<= 1u;
        // save msb of head register as carry flag
        carry = ((union u32 &)head).word[1] >> 15u;
        // shift msb of remainder into 32-bit head register
        head <<= 1u;
        head |= ((union i64 &)rem).word[3] >> 15u;
        rem <<= 1u;
        // compare head to divisor
        if(carry || head >= div) {
            head -= div;
            quo |= 1;
        }
    }

    // sign restoration
    return neg ? -quo : quo;
}

struct FixedMath {
    uint64_t count[BINS];
    int64_t mat[BINS][5];

    FixedMath() : count{}, mat{} {
        bzero(count, sizeof(count));
        bzero(mat, sizeof(mat));
    }

    void update(const Sample &s) {
        int8_t ipart = (int) s.temp;
        int32_t fpart = floorf((s.temp - ipart) * 256.0f) - 128;
        uint8_t tidx = ipart & (BINS-1);
        int32_t ippm = floorf(s.ppm / IPPM_PREC);

        auto &m = mat[tidx];
        accumulate(m[0], (fpart * fpart) << 16);
        accumulate(m[1], fpart << 8);
        accumulate(m[2], ippm * fpart);
        accumulate(m[3], ippm << 8);
        accumulate(m[4], 1);
        ++count[tidx];
    }

    int32_t getCell(uint8_t tidx, uint8_t cidx) {
        if(mat[tidx][4] < (1l << 32))
            return div64s32u(mat[tidx][cidx], mat[tidx][4]);
        else
            return mat[tidx][cidx] >> 32;
    }

    bool coeff(const float temp, int32_t &m, int32_t &b) {
        int8_t ipart = (int) temp;
        uint8_t tidx = ipart & (BINS-1);
        if(mat[tidx][4] == 0) return false;

        int64_t A = getCell(tidx, 0); // 0.32
        int64_t B = getCell(tidx, 1); // 0.16
        int64_t C = getCell(tidx, 2); // 24.8
        int64_t D = getCell(tidx, 3); // 24.8
        // adjust alignment
        C <<= 16; // 24.24

        int32_t Z = A - (B * B); // 0.32
        if(Z <= 0 || count[tidx] < 64) {
            m = 0;
            b = D;
        } else {
            m = div64s32u(((C * 1) - (B * D)) << 16, Z);
            b = div64s32u(((A * D) - (B * C)), Z);
        }
        return true;
    }
} fixedMath;

struct FloatMath {
    float steps[BINS];

    FloatMath() : steps{} {
        bzero(steps, sizeof(steps));
    }

    void update(const Sample &s) {
        auto ibase = (int) (s.temp / TEMP_PREC);
        auto &step = steps[ibase & (BINS-1)];
        if(step == 0) {
            step = s.ppm;
        } else {
            step += (s.ppm - step) / 256;
        }
    }

    bool coeff(const float temp, float &m, float &b, float &o) {
        const int ibase = (int) (temp / TEMP_PREC);
        if(steps[ibase & (BINS-1)] == 0)
            return false;

        o = (ibase + 0.5f) * TEMP_PREC;
        b = steps[ibase & (BINS-1)];
        m = 0;

        int cnt = 0;
        float scratch[TEMP_SPAN * 2];
        for(int i = -TEMP_SPAN; i <= (TEMP_SPAN-1); i++) {
            float l = steps[(ibase + i) & (BINS-1)];
            float u = steps[(ibase + i + 1) & (BINS-1)];
            if(l != 0 && u != 0) {
                scratch[cnt++] = u - l;
            }
        }
        if(cnt == 0)
            return true;

        float mean = 0;
        for(int i = 0; i < cnt; i++)
            mean += scratch[i];
        mean /= cnt;

        float stddev = 0;
        for(int i = 0; i < cnt; i++) {
            auto diff = scratch[i] - mean;
            stddev += diff * diff;
        }
        stddev = sqrtf(stddev / cnt);

        int n = 0;
        for(int i = 0; i < cnt; i++) {
            auto diff = scratch[i] - mean;
            if(fabsf(diff) <= stddev) {
                m += scratch[i];
                ++n;
            }
        }
        if(n > 0) {
            m /= n;
            m /= TEMP_PREC;
        }
        return true;

        // const int ibase = (int) (temp / TEMP_PREC);
        // float norm = 0;
        // for(int i = -(TEMP_SPAN-1); i <= (TEMP_SPAN-1); i++) {
        //     int n = (i < 0) ? (TEMP_SPAN + i) : (TEMP_SPAN - i);
        //     float step = steps[(ibase + i) & (BINS-1)];
        //     if(step != 0) {
        //         o += (ibase + i) * n;
        //         b += n * step;
        //         norm += n;
        //     }
        // }
        // if(norm == 0)
        //     return false;
        // b /= norm;
        // o /= norm;
        // o = TEMP_PREC * (o + 0.5f);

        // norm = 0;
        // for(int i = 0; i <= (TEMP_SPAN-1); i++) {
        //     int n = (i < 0) ? (TEMP_SPAN + i) : (TEMP_SPAN - i);
        //     {
        //         float l = steps[(ibase + i) & (BINS-1)];
        //         float u = steps[(ibase + i + 1) & (BINS-1)];
        //         if(l != 0 && u < l) {
        //             m += n * (u - l);
        //             norm += n;
        //         }
        //     }
        //     {
        //         float u = steps[(ibase - i) & (BINS-1)];
        //         float l = steps[(ibase - i - 1) & (BINS-1)];
        //         if(l != 0 && u < l) {
        //             m += n * (u - l);
        //             norm += n;
        //         }
        //     }
        // }
        // m /= norm;
        // m /= TEMP_PREC;
        // if(norm == 0)
        //     m = 0;
        // return true;
    }
} floatMath;

struct NodeMath {
    struct Node {
        float center[2];
        float cov[2];
    } nodes[16];

    float ndist;
    float tau;
    int cnt;

    NodeMath() {
        bzero(nodes, sizeof(nodes));
        cnt = 0;
        tau = 16;
        ndist = -1;
    }

    void updateNode(Node &node, const Sample &s, float w) {
        const float rate = w / tau;
        float del[2];
        del[0] = rate * (s.temp - node.center[0]);
        del[1] = rate * (s.ppm - node.center[1]);
        node.center[0] += del[0];
        node.center[1] += del[1];

        float diff[2];
        diff[0] = s.temp - node.center[0];
        diff[1] = s.ppm - node.center[1];

        for(int i = 0; i < 2; i++) {
            node.cov[i] += rate * ((diff[0] * diff[i]) - node.cov[i]);
            node.cov[i] += del[0] * del[i];
        }
    }

    void update(const Sample &s) {
        if(cnt == 0) {
            for(int i = 0; i < 16; i++) {
                nodes[i].center[0] = s.temp + (((float)(i-8)) / 16);
                nodes[i].center[1] = s.ppm;
            }
        }

        int best = 0;
        float min = fabsf(s.temp - nodes[best].center[0]);
        for(int i = 1; i < 16; i++) {
            float dist = fabsf(s.temp - nodes[i].center[0]);
            if(dist < min) {
                best = i;
                min = dist;
            }
        }

        if(cnt < 256) ++cnt;
        else {
            tau = 256;
            ndist = -3;
        }
        for(int i = 0; i < 16; i++) {
            int dist = abs(i - best);
            updateNode(nodes[i], s, ldexpf(1, ndist * dist));
        }
    }

    float predict(float temp) {
        if(cnt < 256) return NAN;

        int best = 0;
        float min = fabsf(temp - nodes[best].center[0]);
        for(int i = 1; i < 16; i++) {
            float dist = fabsf(temp - nodes[i].center[0]);
            if(dist < min) {
                best = i;
                min = dist;
            }
        }

        auto &node = nodes[best];
        return (temp - node.center[0]) * (node.cov[1] / node.cov[0]) + node.center[1];
    }

} nodeMath;


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
        row.temp *= 1.0f;
        row.ppm *= 2.1639109f;
        if(row.isValid())
            data.emplace_back(row);
        
        getline(fin, line);
    }
    fin.close();
    cout << fixed << setprecision(6);
    cout << "loaded " << data.size() << " data samples" << endl;
    cout << endl;

    float biasI = 0, biasF = 0;
    float maxI = 0, maxF = 0;
    float rmseI = 0, rmseF = 0;
    size_t cntI = 0, cntF = 0;
    size_t errI = 0, errF = 0;
    for(auto s : data) {
        if(s.error > 250.0f) continue;

        int32_t im, ib;
        if(fixedMath.coeff(s.temp, im, ib)) {
            int8_t it = (int) s.temp;
            int32_t ix = (int)((s.temp - it) * 256) - 128;
            auto iy = (((int64_t)im * ix) + ((int64_t)ib << 8)) >> 16;
            float id = (iy * IPPM_PREC) - s.ppm;
            if(std::isfinite(id)) {
                biasI += id;
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

        // float fm, fb, fo;
        // if(floatMath.coeff(s.temp, fm, fb, fo)) {
        //     int ibase = (int) (s.temp / TEMP_PREC);
        //     auto fx = s.temp - fo;
        //     auto fy = (fm * fx) + fb;
        //     auto fd = fy - s.ppm;
        //     if(std::isfinite(fd)) {
        //         biasF += fd;
        //         rmseF += fd * fd;
        //         ++cntF;
        //     }
        //     maxF = std::max(maxF, std::abs(fd));
        //     if(std::abs(fd) > 0.100) {
        //         cout << s.temp << " " << s.ppm << " " << fy << endl;
        //         ++errF;
        //     }
        // }
        // floatMath.update(s);

        nodeMath.update(s);
        {
            float fy = nodeMath.predict(s.temp);
            auto fd = fy - s.ppm;
            if(std::isfinite(fd)) {
                biasF += fd;
                rmseF += fd * fd;
                ++cntF;
            }
            maxF = std::max(maxF, std::abs(fd));
            if(std::abs(fd) > 0.100) {
                cout << s.temp << " " << s.ppm << " " << fy << endl;
                ++errF;
            }
        }
    }
    cout << "biasI: " << (biasI / cntI) << " ppm" << endl;
    cout << "biasF: " << (biasF / cntF) << " ppm" << endl;
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

    for(int i = 0; i < BINS; i++) {
        if(fixedMath.count[i] == 0) continue;
        cout << setw(3) << i << ": ";
        cout << setw(8) << fixedMath.count[i];
        cout << setw(12) << fixedMath.mat[i][4] / 65536.0f / 65536.0f;
        cout << setw(12) << fixedMath.getCell(i, 0) / 65536.0f / 65536.0f;
        cout << setw(12) << fixedMath.getCell(i, 1) / 65536.0f;
        cout << setw(12) << fixedMath.getCell(i, 2) * IPPM_PREC / 256.0f;
        cout << setw(12) << fixedMath.getCell(i, 3) * IPPM_PREC / 256.0f;

        int32_t m, b;
        fixedMath.coeff(i, m, b);
        cout << setw(12) << (m * IPPM_PREC) / 256.0f;
        cout << setw(12) << (b * IPPM_PREC) / 256.0f;
        cout << endl;
    }
    cout << endl;

    cout << "floating point math:" << endl;
    cout << "Temp.";
    cout << setw(12) << "m";
    cout << setw(12) << "b";
    cout << endl;

    float m, b, o;
    for(int i = 0; i < BINS; i++) {
        if(fixedMath.count[i] == 0) continue;
        cout << setw(3) << i << ": ";
        floatMath.coeff(i, m, b, o);
        cout << setw(12) << m;
        cout << setw(12) << b;
        cout << endl;
    }
    cout << endl;

    float z = 0;
    for(int i = 0; i < 512; i++)
        z += (1 - z) / 4096.0f;
    cout << z << endl;

    for(auto &node : nodeMath.nodes) {
        cout << node.center[0] << " "  << node.center[1];
        cout << " " << node.cov[0];
        cout << " " << node.cov[1];

        cout << " " << node.cov[1] / node.cov[0];
        cout << endl;
    }

    return 0;
}
