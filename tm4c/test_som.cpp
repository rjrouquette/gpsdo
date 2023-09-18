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

void fitGradient(const float * const data, const int cnt, const int dim, float alpha, float *coef) {
    float delta[dim+1];
    for(int j = 0; j <= dim; j++)
        delta[j] = 0;

    for(int i = 0; i < cnt; i++) {
        auto row = data + (i * 3);
        float error = row[1];
        float x = 1;
        for(int j = 0; j <= dim; j++) {
            error -= x * coef[j];
            x *= row[0];
        }

        error *= row[2];
        x = 1;
        for(int j = 0; j <= dim; j++) {
            delta[j] += error * x;
            x *= row[0];
        }
    }

    alpha /= cnt;
    for(int j = 0; j <= dim; j++)
        coef[j] += alpha * delta[j];
}

void fitLinear(const float * const data, const int cnt, float *coef) {
    float xx = 0, xy = 0;
    for(int i = 0; i < cnt; i++) {
        auto row = data + (i * 3);
        float x = row[0];
        float y = row[1];

        xx += x * x * row[2];
        xy += x * y * row[2];
    }
    coef[0] = xy / xx;
}

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

    // re-center data
    float mean[3];
    bzero(mean, sizeof(mean));

    // compute means
    for(int i = 0; i < 16; i++) {
        auto row = scratch + (i * 3);
        mean[0] += row[0] * row[2];
        mean[1] += row[1] * row[2];
        mean[2] += row[2];
    }
    mean[0] /= mean[2];
    mean[1] /= mean[2];

    // re-center data
    for(int i = 0; i < 16; i++) {
        auto row = scratch + (i * 3);
        row[0] -= mean[0];
        row[1] -= mean[1];
    }

    float coef[4];
    bzero(coef, sizeof(coef));
    fitLinear(scratch, 16, coef + 1);
    for (int i = 0; i < 1<<16; i++)
        fitGradient(scratch, 16, 2, 0x1p-12f, coef);

    for (auto c : coef)
        fprintf(stdout, "%f\n", c);

    fprintf(stdout, "plot:\n");
    fprintf(stdout, "temp,som,linear,quadratic,cubic,error\n");
    float rmse = 0;
    for(auto &row : som) {
        fprintf(stdout, "%f,%f", row[0], row[1]);
        float x = row[0] - mean[0];
        float y = coef[0] + mean[1];
        y += x * coef[1];
        fprintf(stdout, ",%f", y);
        y += x * x * coef[2];
        fprintf(stdout, ",%f", y);
        y += x * x * x * coef[3];
        fprintf(stdout, ",%f", y);
        y -= row[1];
        fprintf(stdout, ",%f\n", y);
        rmse += y * y * row[2];
    }
    fflush(stdout);

    rmse = sqrtf(rmse / mean[2]);
    fprintf(stdout, "\nrmse: %f\n", rmse);
    fflush(stdout);

    return 0;
}

/*
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
    rmse /= mean[2];

    for(auto &row : som) {
        float x = row[0] - mean[0];
        float y = coef[0] + mean[1];
        y += x * coef[1];
        y -= row[1];
        row[2] *= expf(-0.25 * (y * y) / rmse);
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
*/