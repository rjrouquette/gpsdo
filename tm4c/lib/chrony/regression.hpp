//
// Created by robert on 5/13/24.
//

#pragma once

namespace chrony {
    bool findBestRegression(
        const float *x, /* independent variable */
        const float *y, /* measured data */
        const float *w, /* weightings (large => data
                                   less reliable) */

        int n,           /* number of data points */
        int m,           /* number of extra samples in x and y arrays
                                   (negative index) which can be used to
                                   extend runs test */
        int min_samples, /* minimum number of samples to be kept after
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
    );
}
