/*
 * unit_test_run_workload.cc: unit tests for pilot_run_workload()
 *
 * Copyright (c) 2015, University of California, Santa Cruz, CA, USA.
 * Created by Yan Li <yanli@ucsc.edu, elliot.li.tech@gmail.com>,
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
#include <cstring>
#include "gtest/gtest.h"
#include "libpilot.h"
#include <vector>

using namespace pilot;
using namespace std;

static int g_test_round = 0;
static const int g_num_of_pi = 1;    //! TODO: need cases with more PIs
/**
 * g_mock_readings[piid][round]
 */
static const vector<vector<double> > g_mock_readings {
    {42}
};
/**
 * g_mock_unit_readings[round][piid][unit_id]
 */
static const vector<vector<vector<double> > > g_mock_unit_readings {
    {{1, 5, 10, 20, 30, 40, 42, 42, 42}}
};
static const int g_total_rounds = g_mock_readings.size();

int mock_workload_func(size_t total_work_amount,
                       pilot_malloc_func_t *lib_malloc_func,
                       size_t *num_of_work_unit,
                       double ***unit_readings,
                       double **readings) {
    assert(500/10 == total_work_amount);
    *num_of_work_unit = g_mock_unit_readings[g_test_round][0].size();

    // store unit_readings
    assert(NULL == *unit_readings);
    *unit_readings = (double**)lib_malloc_func(sizeof(double*) * g_num_of_pi);
    for (int piid = 0; piid < g_num_of_pi; ++piid) {
        size_t memsize = sizeof(double) * g_mock_unit_readings[g_test_round][piid].size();
        (*unit_readings)[piid] = (double*)lib_malloc_func(memsize);
        memcpy((*unit_readings)[piid], g_mock_unit_readings[g_test_round][piid].data(), memsize);
    }

    // store readings
    assert(NULL == *readings);
    *readings = (double*)lib_malloc_func(sizeof(double*) * g_num_of_pi);
    for (int i = 0; i < g_num_of_pi; ++i)
        (*readings)[i] = g_mock_readings[i][g_test_round];

    ++g_test_round;
    return 0;
}

bool post_workload_hook(pilot_workload_t* wl) {
    // we only run a single round
    return false;
}

TEST(PilotRunWorkloadTest, RunWorkload) {
    pilot_workload_t *wl = pilot_new_workload("Test workload");
    size_t num_of_pi;
    pilot_set_log_level(lv_fatal);
    ASSERT_EQ(ERR_NOT_INIT, pilot_get_num_of_pi(wl, &num_of_pi));
    pilot_set_log_level(lv_warning);
    pilot_set_num_of_pi(wl, 1);
    ASSERT_EQ(0, pilot_get_num_of_pi(wl, &num_of_pi));
    ASSERT_EQ(size_t(1), num_of_pi);

    // Limit the write to 500 MB
    pilot_set_work_amount_limit(wl, 500);
    size_t work_amount_limit;
    ASSERT_EQ(0, pilot_get_work_amount_limit(wl, &work_amount_limit));
    ASSERT_EQ(size_t(500), work_amount_limit);

    pilot_set_workload_func(wl, &mock_workload_func);
    pilot_set_hook_func(wl, POST_WORKLOAD_RUN, &post_workload_hook);

    ASSERT_EQ(ERR_STOPPED_BY_HOOK, pilot_run_workload(wl));
    ASSERT_EQ(1, pilot_get_num_of_rounds(wl));
    ASSERT_EQ(0, memcmp(g_mock_readings[0].data(), pilot_get_pi_readings(wl, 0),
                        sizeof(double) * g_mock_readings[0].size()));
    size_t num_of_work_units;
    const double *pi_unit_readings = pilot_get_pi_unit_readings(wl, 0, 0, &num_of_work_units);
    ASSERT_EQ(g_mock_unit_readings[0][0].size(), num_of_work_units);
    ASSERT_EQ(0, memcmp(g_mock_unit_readings[0][0].data(), pi_unit_readings,
                        sizeof(double) * g_mock_unit_readings[0][0].size()));
    ASSERT_EQ(0, pilot_export(wl, CSV, "unit_test_run_workload_export.csv"));
    pilot_destroy_workload(wl);
}

TEST(PilotRunWorkloadTest, ChangingNumberOfReadings) {
    //! TODO: Changing number of readings after running the first round of workload is not allowed.
}

int main(int argc, char **argv) {
    PILOT_LIB_SELF_CHECK;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
