//
// Created by robert on 5/4/23.
//

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

float som[32][3] = {
        32.7826304, -175.420128, 0.99999392,
        33.0986304, -175.271488, 0.99999392,
        33.4473728, -175.095904, 0.99999392,
        33.7490048, -174.952592, 0.999991104,
        33.9023616, -174.879568, 0.999989824,
        33.9834176, -174.836912, 0.999989632,
        34.0608928, -174.809008, 0.999689984,
        34.1380128, -174.784176, 0.996802816,
        34.1936, -174.779776, 0.894894592,
        34.2349312, -174.80208, 0.513017344,
        34.2470464, -174.819824, 0.185899552,
        34.2490528, -174.826576, 0.0570820096,
        34.2493792, -174.827664, 0.038112288,
        34.2496896, -174.826992, 0.0599866688,
        34.2500032, -174.827408, 0.0465335584,
        34.2503168, -174.826768, 0.053177536,
        34.2506304, -174.82736, 0.0416086304,
        34.2509408, -174.827808, 0.0321509696,
        34.2512544, -174.827104, 0.0377498208,
        34.251568, -174.827616, 0.039839088,
        34.2519456, -174.825344, 0.0908039168,
        34.2545664, -174.811856, 0.3305944,
        34.2644512, -174.786224, 0.707310528,
        34.289936, -174.767744, 0.918483328,
        34.3273824, -174.750464, 0.987793088,
        34.3959008, -174.734144, 0.996568768,
        34.4692416, -174.701152, 0.999794752,
        34.5973216, -174.61416, 0.962856384,
        35.1281056, -174.445104, 0.987831936,
        35.915936, -174.127024, 0.99875904,
        36.8295872, -173.712928, 0.999969472,
        37.8952, -173.233344, 0.999969472
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
