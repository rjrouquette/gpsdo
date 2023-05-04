//
// Created by robert on 5/4/23.
//

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

float som[32][3] = {
        32.8174, -175.3559, 0.99997,
        33.2463, -175.1523, 0.99997,
        33.6058, -175.0139, 0.99754,
        33.9429, -174.8766, 0.93317,
        34.1221, -174.8124, 0.81339,
        34.2109, -174.7910, 0.57786,
        34.2392, -174.8065, 0.26705,
        34.2474, -174.8242, 0.06304,
        34.2481, -174.8282, 0.01625,
        34.2484, -174.8289, 0.00852,
        34.2488, -174.8288, 0.00544,
        34.2491, -174.8288, 0.00412,
        34.2494, -174.8287, 0.00438,
        34.2497, -174.8287, 0.00761,
        34.2500, -174.8287, 0.01051,
        34.2503, -174.8286, 0.00807,
        34.2506, -174.8287, 0.00786,
        34.2509, -174.8286, 0.00870,
        34.2513, -174.8286, 0.00820,
        34.2516, -174.8286, 0.01062,
        34.2519, -174.8287, 0.00826,
        34.2522, -174.8286, 0.00920,
        34.2525, -174.8284, 0.01154,
        34.2528, -174.8287, 0.00721,
        34.2531, -174.8288, 0.00743,
        34.2534, -174.8286, 0.00630,
        34.2538, -174.8287, 0.01164,
        34.2553, -174.8269, 0.05874,
        34.3753, -174.8143, 0.25557,
        35.0015, -174.4882, 0.68900,
        36.7050, -173.6617, 0.99563,
        37.1900, -173.4519, 0.99997,
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