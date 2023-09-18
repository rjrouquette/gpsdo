//
// Created by robert on 5/4/23.
//

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

float som[16][3] = {
        38.1113888, -172.392272, 0.999986816,
        38.60752, -172.171296, 0.999984256,
        38.8272064, -172.07312, 0.99997312,
        38.9498336, -172.012256, 0.99997312,
        39.04688, -171.97296, 0.99997312,
        39.2285312, -171.900272, 0.99997312,
        39.475696, -171.796816, 0.99997312,
        39.905552, -171.608272, 0.99997312,
        40.3379456, -171.416832, 0.999973248,
        40.8939616, -171.174208, 0.999973312,
        41.4561856, -170.94584, 0.999973568,
        41.959504, -170.742576, 0.9999856,
        42.6209728, -170.468496, 0.999996544,
        43.763024, -170.015536, 0.999997696,
        44.6834688, -169.66192, 0.999998592,
        45.9950176, -169.162288, 0.999999232
};

void fitQuadratic(const float * const data, const int cnt, float *coef) {
    // compute equation matrix
    float xx[3][4] = {0};
    for (int i = 0; i < cnt; i++) {
        auto row = data + (i * 3);
        const float w = row[2];
        const float x = row[0];
        const float y = row[1];

        // constant terms
        xx[2][2] += w;
        xx[2][3] += w * y;

        // linear terms
        float z = x;
        xx[1][2] += w * z;
        xx[1][3] += w * z * y;

        // quadratic terms
        z *= x;
        xx[0][2] += w * z;
        xx[0][3] += w * z * y;

        // cubic terms
        z *= x;
        xx[0][1] += w * z;

        // quartic terms
        z *= x;
        xx[0][0] += w * z;
    }
    // employ matrix symmetry
    xx[1][0] = xx[0][1];
    xx[1][1] = xx[0][2];
    xx[2][0] = xx[0][2];
    xx[2][1] = xx[1][2];

    // row-echelon reduction
    if(xx[1][0] != 0) {
        const float f = xx[1][0] / xx[0][0];
        xx[1][1] -= f * xx[0][1];
        xx[1][2] -= f * xx[0][2];
        xx[1][3] -= f * xx[0][3];
    }

    // row-echelon reduction
    {
        const float f = xx[2][0] / xx[0][0];
        xx[2][1] -= f * xx[0][1];
        xx[2][2] -= f * xx[0][2];
        xx[2][3] -= f * xx[0][3];
    }

    // row-echelon reduction
    if(xx[2][1] != 0) {
        const float f = xx[2][1] / xx[1][1];
        xx[2][2] -= f * xx[1][2];
        xx[2][3] -= f * xx[1][3];
    }

    // compute coefficients
    coef[0] = xx[2][3] / xx[2][2];
    coef[1] = (xx[1][3] - xx[1][2] * coef[0]) / xx[1][1];
    coef[2] = (xx[0][3] - xx[0][2] * coef[0] - xx[0][1] * coef[1]) / xx[0][0];
}

int main(int argc, char **argv) {
    float scratch[sizeof(som) / sizeof(float)];
    memcpy(scratch, som, sizeof(som));

    float coef[3];
    fitQuadratic(scratch, 16, coef);
    float rmse = 0, norm = 0;
    for(auto &row : som) {
        float x = row[0];
        float y = coef[0];
        y += x * coef[1];
        y += x * x * coef[2];
        y -= row[1];
        rmse += y * y * row[2];
        norm += row[2];
    }
    rmse /= norm;

    for(auto &row : som) {
        float x = row[0];
        float y = coef[0];
        y += x * coef[1];
        y += x * x * coef[2];
        y -= row[1];
        row[2] *= expf(-0.25 * (y * y) / rmse);
    }

    memcpy(scratch, som, sizeof(som));
    fitQuadratic(scratch, 16, coef);
    for(int i = 0; i < 3; i++)
        fprintf(stdout, "%f\n", coef[i]);
    fflush(stdout);

    fprintf(stdout, "plot:\n");
    fprintf(stdout, "temp,som,linear,quadratic\n");
    rmse = 0, norm = 0;
    for(auto &row : som) {
        fprintf(stdout, "%f,%f", row[0], row[1]);
        float x = row[0];
        float y = coef[0];
        y += x * coef[1];
        fprintf(stdout, ",%f", y);
        y += x * x * coef[2];
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
