//
// Created by robert on 5/4/23.
//

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

float som[32][3] = {
        33.1085664, -177.591856, 0.999995712,
        33.6459488, -177.349792, 0.999994368,
        33.967136, -177.199664, 0.999994112,
        34.2010624, -177.12168, 0.999993408,
        34.458832, -177.015536, 0.969298048,
        36.64904, -175.975936, 0.395785472,
        37.703408, -175.477952, 0.084044672,
        37.8850912, -175.415312, 0.0273852864,
        37.902064, -175.41664, 0.0384460032,
        37.9023776, -175.417872, 0.0542886912,
        37.902688, -175.416928, 0.0372738656,
        37.9029984, -175.416784, 0.0335462752,
        37.903312, -175.417312, 0.0399930784,
        37.9036288, -175.418096, 0.0552885632,
        37.9039488, -175.41672, 0.0474705312,
        37.9042944, -175.41768, 0.174975984,
        37.9045632, -175.414672, 1.0,
        37.9048352, -175.416512, 0.167441472,
        37.9051776, -175.418384, 0.0750879936,
        37.9054976, -175.41792, 0.0558950336,
        37.9058144, -175.417712, 0.047656656,
        37.9063008, -175.421264, 0.0899783296,
        37.9086752, -175.436272, 0.28081648,
        37.918992, -175.46512, 0.649520768,
        37.9455456, -175.474944, 0.901226304,
        37.9825888, -175.468848, 0.982708096,
        38.0330304, -175.446416, 0.998504,
        38.0840736, -175.42168, 0.999862272,
        38.1518432, -175.392064, 0.99999872,
        38.244752, -175.360608, 0.999998976,
        38.3276192, -175.330864, 0.999999424,
        38.3963104, -175.293712, 0.999999616
};



int main(int argc, char **argv) {
    // compute means
    float mean[3] = {0, 0, 0};
    for(auto &row : som) {
        mean[0] += row[0] * row[2];
        mean[1] += row[1] * row[2];
        mean[2] += row[2];
    }
    mean[0] /= mean[2];
    mean[1] /= mean[2];
    // display means
    fprintf(stdout, "means: %f %f %f\n", mean[0], mean[1], mean[2]);
    fflush(stdout);

    // compute beta
    float xy = 0, xx = 0;
    for(auto &row : som) {
        float x = row[0] - mean[0];
        float y = row[1] - mean[1];
        xx += (x * x) * row[2];
        xy += (x * y) * row[2];
    }
    float beta = xy / xx;
    float rmse = 0;
    for(auto &row : som) {
        float x = row[0] - mean[0];
        float y = row[1] - mean[1];
        y -= (x * beta);
        rmse += (y * y) * row[2];
    }
    rmse = sqrtf(rmse / mean[2]);
    // display means
    fprintf(stdout, "beta: %f\n", beta);
    fprintf(stdout, "rmse: %f\n", rmse);
    fflush(stdout);

    // compute quadratic term
    xy = 0, xx = 0;
    for(auto &row : som) {
        float x = row[0] - mean[0];
        float y = row[1] - mean[1];
        y -= x * beta;
        x = x * x;
        xx += (x * x) * row[2];
        xy += (x * y) * row[2];
    }
    float quad = xy / xx;
    rmse = 0;
    for(auto &row : som) {
        float x = row[0] - mean[0];
        float y = row[1] - mean[1];
        y -= x * beta;
        y -= x * x * quad;
        rmse += (y * y) * row[2];
    }
    rmse = sqrtf(rmse / mean[2]);
    // display means
    fprintf(stdout, "quad: %f\n", quad);
    fprintf(stdout, "rmse: %f\n", rmse);
    fflush(stdout);

    // compute cubic term
    xy = 0, xx = 0;
    for(auto &row : som) {
        float x = row[0] - mean[0];
        float y = row[1] - mean[1];
        y -= x * beta;
        y -= x * x * quad;
        x = x * x * x;
        xx += (x * x) * row[2];
        xy += (x * y) * row[2];
    }
    float cube = xy / xx;
    rmse = 0;
    for(auto &row : som) {
        float x = row[0] - mean[0];
        float y = row[1] - mean[1];
        y -= x * beta;
        y -= x * x * quad;
        y -= x * x * x * cube;
        rmse += (y * y) * row[2];
    }
    rmse = sqrtf(rmse / mean[2]);
    // display means
    fprintf(stdout, "cube: %f\n", cube);
    fprintf(stdout, "rmse: %f\n", rmse);
    fflush(stdout);

    fprintf(stdout, "plot:\n");
    fprintf(stdout, "temp,som,linear,quadratic,cubic\n");
    for(auto &row : som) {
        fprintf(stdout, "%f,%f", row[0], row[1]);
        float x = row[0] - mean[0];
        float y = mean[1];
        y += x * beta;
        fprintf(stdout, ",%f", y);
        y += x * x * quad;
        fprintf(stdout, ",%f", y);
        y += x * x * x * cube;
        fprintf(stdout, ",%f\n", y);
    }
    fflush(stdout);

    return 0;
}
