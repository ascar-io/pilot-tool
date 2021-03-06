/*
 * unit_test_statistics.cc: unit tests for statistics routines
 *
 * Copyright (c) 2017-2019 Yan Li <yanli@tuneup.ai>. All rights reserved.
 * The Pilot tool and library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License version 2.1 (not any other version) as published by the Free
 * Software Foundation.
 *
 * The Pilot tool and library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program in file lgpl-2.1.txt; if not, see
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *
 * Commit 033228934e11b3f86fb0a4932b54b2aeea5c803c and before were
 * released with the following license:
 * Copyright (c) 2015, 2016, University of California, Santa Cruz, CA, USA.
 * Created by Yan Li <yanli@tuneup.ai>,
 * Department of Computer Science, Baskin School of Engineering.
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

#include <algorithm>
#include <cmath>
#include <fstream>
#include "gtest/gtest.h"
#include "pilot/libpilot.h"
#include <vector>

using namespace pilot;
using namespace std;

nanosecond_type const ONE_SECOND = 1000000000LL;

/**
 * \details These sample response time are taken from [Ferrari78], page 79.
 */
const vector<double> g_response_time {
    1.21, 1.67, 1.71, 1.53, 2.03, 2.15, 1.88, 2.02, 1.75, 1.84, 1.61, 1.35, 1.43, 1.64, 1.52, 1.44, 1.17, 1.42, 1.64, 1.86, 1.68, 1.91, 1.73, 2.18,
    2.27, 1.93, 2.19, 2.04, 1.92, 1.97, 1.65, 1.71, 1.89, 1.70, 1.62, 1.48, 1.55, 1.39, 1.45, 1.67, 1.62, 1.77, 1.88, 1.82, 1.93, 2.09, 2.24, 2.16
};

/**
 * Sample data for testing binomial proportion confidence interval
 */
const vector<double> g_binary_sample {
    1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1
};

TEST(StatisticsUnitTest, CornerCases) {
    ASSERT_DEATH(pilot_subsession_auto_cov_p(g_response_time.data(), 1, 1, 0, ARITHMETIC_MEAN), "") << "Shouldn't be able to calculate covariance for one sample";
    // Shouldn't be able to calculate optimal subsession size for one sample
    ASSERT_EQ(-1, pilot_optimal_subsession_size_p(g_response_time.data(), 1, ARITHMETIC_MEAN));
}

TEST(StatisticsUnitTest, AutocorrelationCoefficient) {
    double sample_mean = pilot_subsession_mean_p(g_response_time.data(), g_response_time.size(), ARITHMETIC_MEAN);
    ASSERT_DOUBLE_EQ(1.756458333333333, sample_mean) << "Mean is wrong";

    ASSERT_DOUBLE_EQ(0.073474423758865273, pilot_subsession_var_p(g_response_time.data(), g_response_time.size(), 1, sample_mean, ARITHMETIC_MEAN)) << "Subsession mean is wrong";
    ASSERT_DOUBLE_EQ(0.046770566452423196, pilot_subsession_auto_cov_p(g_response_time.data(), g_response_time.size(), 1, sample_mean, ARITHMETIC_MEAN)) << "Coverance mean is wrong";
    ASSERT_DOUBLE_EQ(0.63655574361384437, pilot_subsession_autocorrelation_coefficient_p(g_response_time.data(), g_response_time.size(), 1, sample_mean, ARITHMETIC_MEAN)) << "Autocorrelation coefficient is wrong";

    ASSERT_DOUBLE_EQ(0.55892351761172487, pilot_subsession_autocorrelation_coefficient_p(g_response_time.data(), g_response_time.size(), 2, sample_mean, ARITHMETIC_MEAN)) << "Autocorrelation coefficient is wrong";

    ASSERT_DOUBLE_EQ(0.05264711174242424, pilot_subsession_var_p(g_response_time.data(), g_response_time.size(), 4, sample_mean, ARITHMETIC_MEAN)) << "Subsession var is wrong";
    ASSERT_DOUBLE_EQ(0.08230986644266707, pilot_subsession_autocorrelation_coefficient_p(g_response_time.data(), g_response_time.size(), 4, sample_mean, ARITHMETIC_MEAN)) << "Autocorrelation coefficient is wrong";

    ASSERT_DOUBLE_EQ(0.29157062128900485, pilot_subsession_confidence_interval_p(g_response_time.data(), g_response_time.size(), 4, .95, ARITHMETIC_MEAN));

    // testing binomial proportion confidence interval width
    ASSERT_DOUBLE_EQ(0.46566845477273205, pilot_subsession_confidence_interval_p(g_binary_sample.data(), g_binary_sample.size(), 1, .95, ARITHMETIC_MEAN, BINOMIAL_PROPORTION));

    size_t q = 4;
    ASSERT_DOUBLE_EQ(q, pilot_optimal_subsession_size_p(g_response_time.data(), g_response_time.size(), ARITHMETIC_MEAN));

    size_t opt_sample_size;
    ASSERT_EQ(true, pilot_optimal_sample_size_p(g_response_time.data(), g_response_time.size(), sample_mean * 0.1, ARITHMETIC_MEAN, &q, &opt_sample_size, SAMPLE_MEAN, .95, .1));
    ASSERT_EQ(4, q);
    ASSERT_EQ(34, opt_sample_size);

    //! TODO Tests function pilot_est_sample_var_dist_unknown()
}

TEST(StatisticsUnitTest, HarmonicMean) {
    const vector<double> d {1.21, 1.67, 1.71, 1.53, 2.03, 2.15};
    double hm = pilot_subsession_mean_p(d.data(), d.size(), HARMONIC_MEAN);
    ASSERT_DOUBLE_EQ(1.6568334130160711, hm);
}

// There are more WPS linear regression test cases in unit_test_readings_warmup_removal.cc

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegression1) {
    const double exp_alpha = 42;
    const double exp_v = 0.5;       // 0.5 work amount per second
    const vector<size_t> work_amount{50, 100, 150, 200, 250};
    const vector<double> error{20, -9, -18, -25, 30};
    vector<nanosecond_type> t;
    auto p_error = error.begin();
    double exp_ssr = 0;
    for_each(error.begin(), error.end(), [&exp_ssr](double e) {exp_ssr += e*e;});
    for (size_t c : work_amount) {
        t.push_back(ONE_SECOND * ((1.0 / exp_v) * c + exp_alpha + *(p_error++)));
    }
    double alpha, v, v_ci, ssr;
    pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), t.data(),
        1,  // autocorrelation_coefficient_limit
        0,  // duration threshold
        &alpha, &v, &v_ci, &ssr);
    ASSERT_NEAR(exp_ssr, ssr, 10);
    ASSERT_NEAR(44, alpha, 4);
    ASSERT_NEAR(exp_v, v, 0.1);
    ASSERT_NEAR(0.1803, v_ci, 0.01);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegression2) {
    // We generate some mock test data
    const size_t v_wu = 500;
    const size_t v_s  = 30;
    const size_t v_td = 15;
    const nanosecond_type t_su = 10;
    const nanosecond_type t_wu = 100;
    const nanosecond_type t_td = 30;
    vector<size_t> work_amount;
    vector<nanosecond_type> round_duration;
    ofstream of("output.csv");
    of << "work amount,round_duration" << endl;
    for(nanosecond_type t_s = 0; t_s < 2000; t_s += 50) {
        work_amount.push_back(v_wu * t_wu + v_s * t_s + v_td * t_td);
        round_duration.push_back(t_su + t_wu + t_td + t_s);
        of << work_amount.back() << "," << round_duration.back() << endl;
    }
    of.close();

    double alpha, v, v_ci, ssr;
    pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        1,  // autocorrelation_coefficient_limit
        0,  // duration threshold
        &alpha, &v, &v_ci, &ssr);
    ASSERT_NEAR(0, ssr, .001);
    ASSERT_NEAR(-1541.7 / ONE_SECOND, alpha, .1 / ONE_SECOND);
    ASSERT_NEAR(v_s * ONE_SECOND, v, .1);
    ASSERT_NEAR(0, v_ci, .001);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegression3) {
    // Real data test A
    const vector<size_t> work_amount{429497000, 472446000, 515396000, 558346000};
    const vector<nanosecond_type> round_duration{4681140000, 5526190000, 5632120000, 5611980000};
    double alpha, v, v_ci, ssr;
    pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        1,  // autocorrelation_coefficient_limit
        0,  // duration threshold
        &alpha, &v, &v_ci, &ssr);
    ASSERT_NEAR(0.2059332, ssr, 0.001);
    ASSERT_NEAR(2.0296, alpha, .0001);
    ASSERT_NEAR(ONE_SECOND / 6.7485, v, 10000);
    ASSERT_NEAR(15068212467, v_ci, 1);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegression4) {
    // Real data test B
    const vector<size_t> work_amount{429496729, 472446392, 515396064, 558345736, 601295408, 644245080, 687194752, 730144424, 773094096};
    const vector<nanosecond_type> round_duration{5731883327, 5235129386, 5321265550, 5860121124, 6040418744, 6513983890, 6623204911, 6828709974, 7455453108};
    double alpha, v, v_ci, ssr;
    pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        1,  // autocorrelation_coefficient_limit
        0,  // duration threshold
        &alpha, &v, &v_ci, &ssr);
    ASSERT_NEAR(0.59193307, ssr, 0.000001);
    ASSERT_NEAR(2694596476 / ONE_SECOND, alpha, 1);
    ASSERT_NEAR(172572240, v, 1);
    ASSERT_NEAR(114550295, v_ci, 1);
}

TEST(StatisticsUnitTest, TestOfSignificance) {
    // Sample data from http://www.stat.yale.edu/Courses/1997-98/101/meancomp.htm.
    double mean_male = 98.105;
    double mean_female = 98.394;
    double var_male = pow(0.699, 2);
    double var_female = pow(0.743, 2);
    double ci_left, ci_right;
    size_t sample_size_male = 65;
    size_t sample_size_female = 65;
    double p = pilot_p_eq(mean_male, mean_female,
                          sample_size_male, sample_size_female,
                          var_male, var_female,
                          &ci_left, &ci_right);
    ASSERT_NEAR(0.024, p, 0.001);
    ASSERT_NEAR(-0.540, ci_left, 0.001);
    ASSERT_NEAR(-0.039, ci_right, 0.001);

    size_t opt_sample_size;
    ASSERT_EQ(0, pilot_optimal_sample_size_for_eq_test(mean_male,
            sample_size_male, var_male, mean_female, sample_size_female,
            var_female, p, &opt_sample_size));
    ASSERT_EQ(sample_size_female, opt_sample_size);
}

TEST(StatisticsUnitTest, TestSpeedOfLight) {
    // Test data from http://math.arizona.edu/~ghystad/chapter12.pdf
    vector<double> vel1{850, 740, 900,1070, 930, 850, 950, 980, 980, 880,1000, 980, 930, 650, 760,
                        810,1000,1000, 960, 960, 960, 940, 960, 940, 880, 800, 850, 880, 900, 840,
                        830, 790, 810, 880, 880, 830, 800, 790, 760, 800, 880, 880, 880, 860, 720,
                        720, 620, 860, 970, 950, 880, 910, 850, 870, 840, 840, 850, 840, 840, 840,
                        890, 810, 810, 820, 800, 770, 760, 740, 750, 760, 910, 920, 890, 860, 880,
                        720, 840, 850, 850, 780, 890, 840, 780, 810, 760, 810, 790, 810, 820, 850,
                        870, 870, 810, 740, 810, 940, 950, 800, 810, 870};
    vector<double> vel2{883, 816, 778, 796, 682, 711, 611, 599,1051, 781, 578, 796, 774, 820, 772,
                        696, 573, 748, 748, 797, 851, 809, 723};
    double mean1 = pilot_subsession_mean_p(vel1.data(), vel1.size(), ARITHMETIC_MEAN);
    double var1 = pilot_subsession_var_p(vel1.data(), vel1.size(), 1, mean1, ARITHMETIC_MEAN);
    double mean2 = pilot_subsession_mean_p(vel2.data(), vel2.size(), ARITHMETIC_MEAN);
    double var2 = pilot_subsession_var_p(vel2.data(), vel2.size(), 1, mean2, ARITHMETIC_MEAN);
    ASSERT_NEAR(pow(79.0105478191, 2), var1, 0.0001);
    ASSERT_NEAR(pow(107.114618526, 2), var2, 0.0001);
    ASSERT_NEAR(27.754, pilot_calc_deg_of_freedom(var1, var2, vel1.size(), vel2.size()), 0.001);
    double ci_left, ci_right;
    ASSERT_NEAR(0.0003625357,
                pilot_p_eq(mean1, mean2, vel1.size(), vel2.size(), var1, var2, &ci_left, &ci_right, 0.99),
                0.0000000001);
    ASSERT_NEAR(30.67544, ci_left, 0.00001);
    ASSERT_NEAR(161.68977, ci_right, 0.00001);
}

TEST(StatisticsUnitTest, TestChangeInMean1) {
    vector<double> data;
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(5.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    int *changepoints;
    size_t cp_n;
    ASSERT_EQ(0, pilot_changepoint_detection(data.data(), data.size(), &changepoints, &cp_n));
    ASSERT_EQ(2, cp_n);
    ASSERT_EQ(30, changepoints[0]);
    ASSERT_EQ(60, changepoints[1]);
    pilot_free(changepoints);
}

TEST(StatisticsUnitTest, FindDominantSegment) {
    vector<double> data;
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(5.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    size_t begin, end;
    ASSERT_EQ(ERR_NO_DOMINANT_SEGMENT, pilot_find_dominant_segment(data.data(), data.size(), &begin, &end));

    data.clear();
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    for (int i = 0; i < 130; ++i)
        data.push_back(5.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    ASSERT_EQ(0, pilot_find_dominant_segment(data.data(), data.size(), &begin, &end));
    // due to the quirks of EDM, these changepoints are approximate, thus the
    // 30, 131 here don't have special meanings but just the output of EDM
    ASSERT_EQ(30, begin);
    ASSERT_EQ(131, end);
}

TEST(StatisticsUnitTest, FindChangepoint) {
    vector<double> data;
    size_t loc;
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    ASSERT_EQ(ERR_NO_CHANGEPOINT, pilot_find_one_changepoint(data.data(), data.size(), &loc));
    for (int i = 0; i < 30; ++i)
        data.push_back(5.1);
    ASSERT_EQ(0, pilot_find_one_changepoint(data.data(), data.size(), &loc));
    ASSERT_EQ(30, loc);
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    ASSERT_EQ(0, pilot_find_one_changepoint(data.data(), data.size(), &loc));
    ASSERT_EQ(60, loc);
}

int main(int argc, char **argv) {
    PILOT_LIB_SELF_CHECK;
    // we only display fatals because errors are expected in some test cases
    pilot_set_log_level(lv_fatal);

    // this does away a gtest warning message, and we don't care about execution time
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
