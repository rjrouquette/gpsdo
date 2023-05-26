//
// Created by robert on 5/4/23.
//

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

float som[16][3] = {
        35.4101632, -176.783888, 0.999986816,
        36.5221952, -175.763024, 0.999984256,
        36.8832832, -175.241488, 0.99997312,
        37.0535616, -175.1744, 0.99997312,
        37.4759648, -174.996112, 0.99997312,
        37.9842272, -174.803344, 0.99997312,
        38.5020384, -174.706112, 0.99997312,
        38.9364288, -174.572928, 0.99997312,
        39.5659328, -174.247584, 0.999973248,
        40.3481888, -173.858384, 0.999973312,
        41.0989376, -173.529728, 0.999973568,
        41.7298912, -173.375584, 0.9999856,
        42.4995968, -173.029376, 0.999996544,
        42.9004512, -172.737168, 0.999997696,
        43.4969888, -172.517664, 0.999998592,
        44.2094272, -172.198032, 0.999999232
};

void fitLinear(const float * const data, const int cnt, float *coef, float *mean) {
    // compute means
    mean[0] = 0;
    mean[1] = 0;
    mean[2] = 0;
    for(int i = 0; i < cnt; i++) {
        auto row = data + (i * 3);
        mean[0] += row[0] * row[2];
        mean[1] += row[1] * row[2];
        mean[2] += row[2];
    }
    mean[0] /= mean[2];
    mean[1] /= mean[2];

    float xx = 0, xy = 0;
    for(int i = 0; i < cnt; i++) {
        auto row = data + (i * 3);
        float x = row[0] - mean[0];
        float y = row[1] - mean[1];

        xx += x * x * row[2];
        xy += x * y * row[2];
    }
    coef[0] = xy / xx;
}

void fitTaylor(const int order, float * const data, const int cnt, float *coef, float *mean) {
    fitLinear(data, cnt, coef + 1, mean);

    // re-center data
    for(int i = 0; i < cnt; i++) {
        auto row = data + (i * 3);
        row[0] -= mean[0];
        row[1] -= mean[1];
    }

    const float rate = 0x1p-64f / mean[2];
    float x[order + 1];
    for(int pass = 0; pass < 256; pass++) {
        for(int i = 0; i < cnt; i++) {
            auto row = data + (i * 3);

            // compute x and error terms
            float error = row[1] - coef[0];
            x[0] = 1;
            for(int j = 1; j <= order; j++) {
                x[j] = x[j - 1] * row[0];
                error -= coef[j] * x[j];
            }

            error *= rate * row[2];
            for(int j = 0; j <= order; j++)
                coef[j] += error * x[j];
        }
    }
}


int main(int argc, char **argv) {
    float scratch[sizeof(som) / sizeof(float)];
    memcpy(scratch, som, sizeof(som));

    float mean[3];
    float coef[4];
    bzero(coef, sizeof(coef));
    fitLinear(scratch, 16, coef + 1, mean);
    float rmse = 0, norm = 0;
    for(auto &row : som) {
        float x = row[0] - mean[0];
        float y = coef[0] + mean[1];
        y += x * coef[1];
        y -= row[1];
        rmse += y * y * row[2];
    }
    rmse = sqrtf(rmse / mean[2]);
    for(auto &row : som) {
        float x = row[0] - mean[0];
        float y = coef[0] + mean[1];
        y += x * coef[1];
        y -= row[1];
        if(fabsf(y) > rmse * 2)
            row[2] = 0;
    }

    memcpy(scratch, som, sizeof(som));
//    bzero(coef, sizeof(coef));
//    fitTaylor(3, scratch, 16, coef, mean);
    fitLinear(scratch, 16, coef + 1, mean);
    fprintf(stdout, "%f\n", mean[2]);
    for(int i = 0; i < 4; i++)
        fprintf(stdout, "%f\n", coef[i]);
    fflush(stdout);

    fprintf(stdout, "plot:\n");
    fprintf(stdout, "temp,som,linear,quadratic,cubic\n");
    rmse = 0, norm = 0;
    for(auto &row : som) {
        fprintf(stdout, "%f,%f", row[0], row[1]);
        float x = row[0] - mean[0];
        float y = coef[0] + mean[1];
        y += x * coef[1];
        fprintf(stdout, ",%f", y);
        y += x * x * coef[2];
        fprintf(stdout, ",%f", y);
        y += x * x * x * coef[3];
        fprintf(stdout, ",%f\n", y);
        y -= row[1];
        rmse += y * y * row[2];
    }
    fflush(stdout);

    rmse = sqrtf(rmse / mean[2]);
    fprintf(stdout, "\nrmse: %f\n", rmse);
    fflush(stdout);

    return 0;
}
