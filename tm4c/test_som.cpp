//
// Created by robert on 5/4/23.
//

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

float som[32][3] = {
        36.6161408, -176.7364, 4.77146656e-25,
        36.6164544, -176.7364, 3.52566176e-24,
        36.616768, -176.7364, 2.60513136e-23,
        36.6170816, -176.7364, 1.92494672e-22,
        36.617392, -176.7364, 1.42235424e-21,
        36.6177056, -176.7364, 1.05098472e-20,
        36.6180192, -176.7364, 7.7657856e-20,
        36.6183328, -176.7364, 5.73818368e-19,
        36.6186432, -176.7364, 4.23997792e-18,
        36.6189568, -176.7364, 3.13294208e-17,
        36.6192672, -176.7364, 2.3149488e-16,
        36.6195808, -176.7364, 1.71052848e-15,
        36.6198912, -176.7364, 1.26391912e-14,
        36.6202048, -176.7364, 9.3391712e-14,
        36.6205184, -176.7364, 6.90076864e-13,
        36.620832, -176.7364, 5.09901344e-12,
        36.6211424, -176.7364, 3.76769024e-11,
        36.621456, -176.7364, 2.783968e-10,
        36.6217696, -176.7364, 2.05708864e-9,
        36.6220832, -176.7364, 1.5199952e-8,
        36.6223936, -176.7364, 1.1231328e-7,
        36.6227072, -176.7364, 8.2988864e-7,
        36.6230208, -176.7364, 6.13207872e-6,
        36.623328, -176.736416, 4.53093824e-5,
        36.6236416, -176.736048, 0.00033474544,
        36.6243072, -176.7312, 0.00247082928,
        36.628768, -176.695488, 0.018114624,
        36.6550432, -176.450608, 0.126399936,
        36.7750656, -175.330848, 0.632691648,
        36.8541664, -174.838, 0.999486912,
        36.8716768, -174.774128, 0.99999968,
        36.8909312, -174.520032, 1.0
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
