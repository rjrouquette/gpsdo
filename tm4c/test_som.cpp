//
// Created by robert on 5/4/23.
//

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

float som[32][3] = {
        32.9097088, -177.568304, 0.999995712,
        34.2429056, -177.150288, 0.999994368,
        34.8764448, -176.973184, 0.999994112,
        35.1022848, -176.865856, 0.999993408,
        35.2886432, -176.785408, 0.999992576,
        35.5061952, -176.688976, 0.99999168,
        35.6948864, -176.611312, 0.999990656,
        35.8604384, -176.53584, 0.999989568,
        36.0451136, -176.456624, 0.999986816,
        36.3589376, -176.309024, 0.999983232,
        36.6957536, -176.153984, 0.999983168,
        36.9247392, -176.043216, 0.9999792,
        37.1846432, -175.928528, 0.999975744,
        37.383536, -175.833664, 0.999974528,
        37.584736, -175.740304, 0.999563584,
        37.7406304, -175.659232, 0.997710528,
        37.8324352, -175.600304, 1.0,
        37.8844672, -175.532896, 0.774019008,
        37.9026976, -175.469808, 0.425777632,
        37.907616, -175.471424, 0.430976608,
        37.9229216, -175.52832, 0.794040128,
        37.9606816, -175.549264, 0.961230464,
        38.0343712, -175.529008, 0.998503616,
        38.1254112, -175.48248, 0.999974272,
        38.2010208, -175.445104, 0.999977792,
        38.3280256, -175.400528, 0.999979648,
        38.5391808, -175.298592, 0.999981376,
        38.679296, -175.229424, 0.999984512,
        38.7835968, -175.194496, 0.99999872,
        39.0230144, -175.101392, 0.999998976,
        39.3463744, -174.938608, 0.999999424,
        39.5651712, -174.853472, 0.999999616
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
    fitTaylor(1, scratch, 32, coef);
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
    fitTaylor(3, scratch, 32, coef);
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
