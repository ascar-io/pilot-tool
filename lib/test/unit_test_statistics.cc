/*
 * unit_test_statistics.cc: unit tests for statistics routines
 *
 * Copyright (c) 2015, University of California, Santa Cruz, CA, USA.
 *
 * Developers:
 *   Yan Li <yanli@cs.ucsc.edu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Storage Systems Research Center, the
 *       University of California, nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OF THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest/gtest.h"
#include "libpilot.h"
#include <vector>

using namespace std;

/**
 * \details These sample response time are taken from [Ferrari78], page 79.
 */
const vector<double> g_mean_response_time{
    1.21, 1.67, 1.71, 1.53, 2.03, 2.15, 1.88, 2.02, 1.75, 1.84, 1.61, 1.35, 1.43, 1.64, 1.52, 1.44, 1.17, 1.42, 1.64, 1.86, 1.68, 1.91, 1.73, 2.18,
    2.27, 1.93, 2.19, 2.04, 1.92, 1.97, 1.65, 1.71, 1.89, 1.70, 1.62, 1.48, 1.55, 1.39, 1.45, 1.67, 1.62, 1.77, 1.88, 1.82, 1.93, 2.09, 2.24, 2.16
};

TEST(StatisticsUnitTest, AutocorrelationCoefficient) {
    double sample_mean = pilot_subsession_mean(g_mean_response_time.data(), g_mean_response_time.size());
    ASSERT_DOUBLE_EQ(1.756458333333333, sample_mean) << "Mean is wrong";

    ASSERT_DOUBLE_EQ(0.073474423758865273, pilot_subsession_var(g_mean_response_time.data(), g_mean_response_time.size(), 1, sample_mean)) << "Subsession mean is wrong";
    ASSERT_DOUBLE_EQ(0.046770566452423196, pilot_subsession_cov(g_mean_response_time.data(), g_mean_response_time.size(), 1, sample_mean)) << "Coverance mean is wrong";
    ASSERT_DOUBLE_EQ(0.63655574361384437, pilot_subsession_autocorrelation_coefficient(g_mean_response_time.data(), g_mean_response_time.size(), 1, sample_mean)) << "Autocorrelation coefficient is wrong";

    ASSERT_DOUBLE_EQ(0.55892351761172487, pilot_subsession_autocorrelation_coefficient(g_mean_response_time.data(), g_mean_response_time.size(), 2, sample_mean)) << "Autocorrelation coefficient is wrong";

    ASSERT_DOUBLE_EQ(0.05264711174242424, pilot_subsession_var(g_mean_response_time.data(), g_mean_response_time.size(), 4, sample_mean)) << "Subsession var is wrong";
    ASSERT_DOUBLE_EQ(0.08230986644266707, pilot_subsession_autocorrelation_coefficient(g_mean_response_time.data(), g_mean_response_time.size(), 4, sample_mean)) << "Autocorrelation coefficient is wrong";

    ASSERT_DOUBLE_EQ(0.066625214255877543, pilot_subsession_confidence_interval(g_mean_response_time.data(), g_mean_response_time.size(), 4, .95));

    ASSERT_DOUBLE_EQ(4, pilot_optimal_subsession_size(g_mean_response_time.data(), g_mean_response_time.size()));

    ASSERT_DOUBLE_EQ(34, pilot_optimal_length(g_mean_response_time.data(), g_mean_response_time.size(), sample_mean * 0.05, .95, .1));

    //! TODO Tests function pilot_est_sample_var_dist_unknown()
}

int main(int argc, char **argv) {
    pilot_set_log_level(warning);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
