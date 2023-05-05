//
// Created by robert on 5/4/23.
//

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

float som[32][3] = {
        32.8547424, -175.343408, 0.9999696,
        33.3704864, -175.127872, 0.999977024,
        33.7352192, -175.067968, 0.999750464,
        33.9151232, -174.9784, 0.982442112,
        34.077232, -174.908272, 0.94451104,
        34.1758432, -174.866976, 0.848873408,
        34.2252352, -174.836784, 0.530205856,
        34.244128, -174.832848, 0.206075376,
        34.2476512, -174.829456, 0.0380080224,
        34.2484384, -174.828928, 0.0115308088,
        34.248752, -174.8288, 0.00599272192,
        34.2490656, -174.828816, 0.00527497568,
        34.2493792, -174.82904, 0.012478764,
        34.2496896, -174.828736, 0.00872448896,
        34.2500032, -174.828672, 0.0108286584,
        34.2503168, -174.828592, 0.00933756928,
        34.2506304, -174.829072, 0.0170658608,
        34.2509408, -174.829024, 0.0188178128,
        34.2512544, -174.828992, 0.0174851552,
        34.251568, -174.82864, 0.0118845936,
        34.2518816, -174.828688, 0.00853240448,
        34.252192, -174.828624, 0.0099571904,
        34.2525216, -174.828448, 0.0169824512,
        34.2544096, -174.829776, 0.046938864,
        34.2665376, -174.829776, 0.174847904,
        34.335632, -174.815376, 0.589273728,
        34.4883744, -174.739376, 0.881799488,
        34.6844992, -174.654176, 0.898053888,
        35.2227424, -174.422896, 0.986076288,
        35.942784, -174.122, 0.998758464,
        36.8357248, -173.713808, 0.999969472,
        37.8955232, -173.233344, 0.999969472
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
