//
// Created by robert on 5/13/24.
//

#include "regression.hpp"
#include "../ntp/Source.hpp"

#include <cmath>


static constexpr int MAX_POINTS = ntp::Source::MAX_HISTORY;
static constexpr int REGRESS_RUNS_RATIO = 2;
static constexpr int MIN_SAMPLES_FOR_REGRESS = 3;

/**
 * Critical value for number of runs of residuals with same sign.
 * 5% critical region for now. (from chrony regress.c)
*/
static constexpr char lutCriticalRuns[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 2, 3,
    3, 3, 4, 4, 5, 5, 5, 6, 6, 7,
    7, 7, 8, 8, 9, 9, 9, 10, 10, 11,
    11, 11, 12, 12, 13, 13, 14, 14, 14, 15,
    15, 16, 16, 17, 17, 18, 18, 18, 19, 19,
    20, 20, 21, 21, 21, 22, 22, 23, 23, 24,
    24, 25, 25, 26, 26, 26, 27, 27, 28, 28,
    29, 29, 30, 30, 30, 31, 31, 32, 32, 33,
    33, 34, 34, 35, 35, 35, 36, 36, 37, 37,
    38, 38, 39, 39, 40, 40, 40, 41, 41, 42,
    42, 43, 43, 44, 44, 45, 45, 46, 46, 46,
    47, 47, 48, 48, 49, 49, 50, 50, 51, 51,
    52, 52, 52, 53, 53, 54, 54, 55, 55, 56
};

static int n_runs_from_residuals(const float *resid, const int n) {
    int nruns = 1;
    for (int i = 1; i < n; i++) {
        if (
            ((resid[i - 1] < 0.0f) && (resid[i] < 0.0f)) ||
            ((resid[i - 1] > 0.0f) && (resid[i] > 0.0f))
        )
            continue;
        ++nruns;
    }
    return nruns;
}

/* ================================================== */
/* Return a boolean indicating whether we had enough points for
   regression */

bool chrony::findBestRegression(
    const float *x, /* independent variable */
    const float *y, /* measured data */
    const float *w, /* weightings (large => data
                                   less reliable) */

    const int n,           /* number of data points */
    const int m,           /* number of extra samples in x and y arrays
                                   (negative index) which can be used to
                                   extend runs test */
    const int min_samples, /* minimum number of samples to be kept after
                                   changing the starting index to pass the runs
                                   test */

    /* And now the results */

    float *b0, /* estimated y axis intercept */
    float *b1, /* estimated slope */
    float *s2, /* estimated variance of data points */

    float *sb0, /* estimated standard deviation of intercept */
    float *sb1, /* estimated standard deviation of slope */

    int *new_start, /* the new starting index to make the residuals pass the two tests */

    int *n_runs, /* number of runs amongst the residuals */

    int *dof /* degrees of freedom in statistics (needed to get confidence intervals later) */
) {
    float Q, U, V, W; /* total */
    float resid[MAX_POINTS * REGRESS_RUNS_RATIO];
    float a, b, u;

    int resid_start, nruns;
    int i;

    // assert(n <= MAX_POINTS && m >= 0);
    // assert(n * REGRESS_RUNS_RATIO < std::size(lutCriticalRuns));

    if (n < MIN_SAMPLES_FOR_REGRESS) {
        return false;
    }

    int start = 0;
    for (;;) {
        W = U = 0;
        for (i = start; i < n; i++) {
            U += x[i] / w[i];
            W += 1.0f / w[i];
        }

        u = U / W;

        float P = Q = V = 0.0;
        for (i = start; i < n; i++) {
            const float ui = x[i] - u;
            P += y[i] / w[i];
            Q += y[i] * ui / w[i];
            V += ui * ui / w[i];
        }

        b = Q / V;
        a = (P / W) - (b * u);

        /* Get residuals also for the extra samples before start */
        resid_start = n - (n - start) * REGRESS_RUNS_RATIO;
        if (resid_start < -m)
            resid_start = -m;

        for (i = resid_start; i < n; i++) {
            resid[i - resid_start] = y[i] - a - b * x[i];
        }

        /* Count number of runs */
        nruns = n_runs_from_residuals(resid, n - resid_start);

        if (nruns > lutCriticalRuns[n - resid_start] ||
            n - start <= MIN_SAMPLES_FOR_REGRESS ||
            n - start <= min_samples) {
            if (start != resid_start) {
                /* Ignore extra samples in returned nruns */
                nruns = n_runs_from_residuals(resid + (start - resid_start), n - start);
            }
            break;
        }

        /* Try dropping one sample at a time until the runs test passes. */
        ++start;
    }

    /* Work out statistics from full dataset */
    *b1 = b;
    *b0 = a;

    float ss = 0.0;
    for (i = start; i < n; i++) {
        ss += resid[i - resid_start] * resid[i - resid_start] / w[i];
    }

    const int npoints = n - start;
    ss /= static_cast<float>(npoints - 2);
    *sb1 = std::sqrt(ss / V);
    const float aa = u * (*sb1);
    *sb0 = std::sqrt((ss / W) + (aa * aa));
    *s2 = ss * static_cast<float>(npoints) / W;

    *new_start = start;
    *dof = npoints - 2;
    *n_runs = nruns;

    return true;
}
