//
// Created by robert on 5/4/23.
//

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

float som[16][3] = {
        35.41568, -176.835712, 0.999986816,
        35.86736, -176.774224, 0.999984256,
        36.097232, -176.67496, 0.99997312,
        36.3899584, -176.546192, 0.99997312,
        36.6389056, -176.430352, 0.99997312,
        36.9589472, -176.285408, 0.99997312,
        38.5345856, -175.361008, 0.99997312,
        38.7093056, -175.223456, 0.99997312,
        38.848576, -175.173248, 0.999973248,
        39.2331744, -175.03536, 0.999973312,
        39.5480128, -174.903152, 0.999973568,
        40.0945472, -174.639136, 0.9999856,
        42.4293088, -173.654432, 0.999996544,
        43.0228736, -173.41936, 0.999997696,
        43.439056, -173.241088, 0.999998592,
        44.1988256, -172.913424, 0.999999232
};


void fitTaylor(const int order, float * const data, const int cnt, float *coef) {
    // compute means
    float mean[3] = {0, 0, 0};
    for(int i = 0; i < cnt; i++) {
        auto row = data + (i * 3);
        mean[0] += row[0] * row[2];
        mean[1] += row[1] * row[2];
        mean[2] += row[2];
    }
    mean[0] /= mean[2];
    mean[1] /= mean[2];

    // re-center data
    for(int i = 0; i < cnt; i++) {
        auto row = data + (i * 3);
        row[0] -= mean[0];
        row[1] -= mean[1];
    }
    // set offsets
    coef[0] = mean[0];
    coef[1] = mean[1];

    if(order >= 1) {
        // compute linear coefficient
        float xx = 0, xy = 0;
        for(int i = 0; i < cnt; i++) {
            auto row = data + (i * 3);
            xx += row[2] * row[0] * row[0];
            xy += row[2] * row[1] * row[0];
        }
        coef[2] = xy / xx;

        // re-center data
        for(int i = 0; i < cnt; i++) {
            auto row = data + (i * 3);
            row[1] -= coef[2] * row[0];
        }
    }

    // compute coefficients
    for(int o = 2; o <= order; o++) {
        float xx[2] = {0,0};
        float xy[2] = {0,0};

        // compute covariance
        for(int i = 0; i < cnt; i++) {
            auto row = data + (i * 3);
            float x = row[0];
            for(int j = 1; j < o; j++)
                x *= row[0];

            xx[0] += row[2] * x;
            xx[1] += row[2] * x * x;
            xy[0] += row[2] * row[1];
            xy[1] += row[2] * x * row[1];
        }

        const float z = (mean[2] * xx[1]) - (xx[0] * xx[0]);
        const float a = ((xy[0] * xx[1]) - (xy[1] * xx[0])) / z;
        const float b = ((mean[2] * xy[1]) - (xy[0] * xx[0])) / z;
        coef[o+1] = b;
        coef[1] += a;

        // re-center data
        for(int i = 0; i < cnt; i++) {
            auto row = data + (i * 3);
            float x = row[0];
            for(int j = 1; j < o; j++)
                x *= row[0];
            row[1] -= a + (b * x);
        }
    }
}


int main(int argc, char **argv) {
//    // compute means
//    float mean[3] = {0, 0, 0};
//    for(auto &row : som) {
//        mean[0] += row[0] * row[2];
//        mean[1] += row[1] * row[2];
//        mean[2] += row[2];
//    }
//    mean[0] /= mean[2];
//    mean[1] /= mean[2];
//    // display means
//    fprintf(stdout, "means: %f %f %f\n", mean[0], mean[1], mean[2]);
//    fflush(stdout);
//
//    // compute beta
//    float xy = 0, xx = 0;
//    for(auto &row : som) {
//        float x = row[0] - mean[0];
//        float y = row[1] - mean[1];
//        xx += (x * x) * row[2];
//        xy += (x * y) * row[2];
//    }
//    float beta = xy / xx;
//    float rmse = 0;
//    for(auto &row : som) {
//        float x = row[0] - mean[0];
//        float y = row[1] - mean[1];
//        y -= (x * beta);
//        rmse += (y * y) * row[2];
//    }
//    rmse = sqrtf(rmse / mean[2]);
//    // display means
//    fprintf(stdout, "beta: %f\n", beta);
//    fprintf(stdout, "rmse: %f\n", rmse);
//    fflush(stdout);
//
//    // compute quadratic term
//    xy = 0, xx = 0;
//    for(auto &row : som) {
//        float x = row[0] - mean[0];
//        float y = row[1] - mean[1];
//        y -= x * beta;
//        x = x * x;
//        xx += (x * x) * row[2];
//        xy += (x * y) * row[2];
//    }
//    float quad = xy / xx;
//    rmse = 0;
//    for(auto &row : som) {
//        float x = row[0] - mean[0];
//        float y = row[1] - mean[1];
//        y -= x * beta;
//        y -= x * x * quad;
//        rmse += (y * y) * row[2];
//    }
//    rmse = sqrtf(rmse / mean[2]);
//    // display means
//    fprintf(stdout, "quad: %f\n", quad);
//    fprintf(stdout, "rmse: %f\n", rmse);
//    fflush(stdout);
//
//    // compute cubic term
//    xy = 0, xx = 0;
//    for(auto &row : som) {
//        float x = row[0] - mean[0];
//        float y = row[1] - mean[1];
//        y -= x * beta;
//        y -= x * x * quad;
//        x = x * x * x;
//        xx += (x * x) * row[2];
//        xy += (x * y) * row[2];
//    }
//    float cube = xy / xx;
//    rmse = 0;
//    for(auto &row : som) {
//        float x = row[0] - mean[0];
//        float y = row[1] - mean[1];
//        y -= x * beta;
//        y -= x * x * quad;
//        y -= x * x * x * cube;
//        rmse += (y * y) * row[2];
//    }
//    rmse = sqrtf(rmse / mean[2]);
//    // display means
//    fprintf(stdout, "cube: %f\n", cube);
//    fprintf(stdout, "rmse: %f\n", rmse);
//    fflush(stdout);
//
//    fprintf(stdout, "plot:\n");
//    fprintf(stdout, "temp,som,linear,quadratic,cubic\n");
//    for(auto &row : som) {
//        fprintf(stdout, "%f,%f", row[0], row[1]);
//        float x = row[0] - mean[0];
//        float y = mean[1];
//        y += x * beta;
//        fprintf(stdout, ",%f", y);
//        y += x * x * quad;
//        fprintf(stdout, ",%f", y);
//        y += x * x * x * cube;
//        fprintf(stdout, ",%f\n", y);
//    }
//    fflush(stdout);

    float scratch[sizeof(som) / sizeof(float)];
    memcpy(scratch, som, sizeof(som));

    float coef[5];
    bzero(coef, sizeof(coef));
    fitTaylor(1, scratch, 16, coef);
    float rmse = 0, norm = 0;
    for(auto &row : som) {
        float x = row[0] - coef[0];
        float y = coef[1];
        y += x * coef[2];
        y -= row[1];
        rmse += y * y * row[2];
        norm += row[2];
    }
    rmse = sqrtf(rmse / norm);
    for(auto &row : som) {
        float x = row[0] - coef[0];
        float y = coef[1];
        y += x * coef[2];
        y -= row[1];
        if(fabsf(y) > rmse * 2)
            row[2] = 0;
    }

    memcpy(scratch, som, sizeof(som));
    bzero(coef, sizeof(coef));
    fitTaylor(3, scratch, 16, coef);
    for(int i = 0; i < 5; i++)
        fprintf(stdout, "%f\n", coef[i]);
    fflush(stdout);

    fprintf(stdout, "plot:\n");
    fprintf(stdout, "temp,som,linear,quadratic,cubic\n");
    rmse = 0, norm = 0;
    for(auto &row : som) {
        fprintf(stdout, "%f,%f", row[0], row[1]);
        float x = row[0] - coef[0];
        float y = coef[1];
        y += x * coef[2];
        fprintf(stdout, ",%f", y);
        y += x * x * coef[3];
        fprintf(stdout, ",%f", y);
        y += x * x * x * coef[4];
        fprintf(stdout, ",%f\n", y);
        y -= row[1];
        rmse += y * y * row[2];
        norm += row[2];
    }
    fflush(stdout);

    rmse = sqrtf(rmse / norm);
    fprintf(stdout, "\nrmse: %f\n", rmse);
    fflush(stdout);

    return 0;
}
