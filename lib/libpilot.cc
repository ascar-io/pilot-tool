/*
 * libpilot.cc: routines for handling workloads
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

#include "common.h"
#include "config.h"
#include "interface_include/libpilot.h"
#include <vector>

using namespace pilot;
using namespace std;

struct pilot_workload_t {
    typedef vector<double> reading_data_t; //! The data of one reading of all rounds
    typedef vector<double> unit_reading_data_per_round_t;
    typedef vector<unit_reading_data_per_round_t> unit_reading_data_t; //! Per round unit reading data

    size_t num_of_pi;                      //! Number of performance indices to collect for each round
    size_t rounds;                         //! Number of rounds we've done so far
    size_t total_work_amount;
    pilot_workload_func_t *workload_func;
    vector<reading_data_t> readings;       //! Reading data of each round. Format: readings[piid][round_id].
    vector<unit_reading_data_t> unit_readings; //! Unit reading data of each round. Format: unit_readings[piid][round_id].

    pilot_workload_t() : num_of_pi(0), rounds(0), total_work_amount(0),
                         workload_func(nullptr) {}
};

pilot_workload_t* pilot_new_workload(const char *workload_name) {
    pilot_workload_t *wl = new pilot_workload_t;
    return wl;
}

void pilot_set_num_of_pi(pilot_workload_t* wl, size_t num_of_readings) {
    wl->num_of_pi = num_of_readings;
    wl->readings.resize(num_of_readings);
    wl->unit_readings.resize(num_of_readings);
}

void pilot_set_workload_func(pilot_workload_t* wl, pilot_workload_func_t *wf) {
    wl->workload_func = wf;
}

void pilot_set_total_work_amount(pilot_workload_t* wl, size_t t) {
    wl->total_work_amount = t;
}

int pilot_run_workload(pilot_workload_t *wl) {
    // sanity check
    if (wl->workload_func == nullptr || wl->num_of_pi == 0 ||
        wl->total_work_amount == 0) return 11;
    // ready to start the workload
    size_t num_of_work_units;
    double **unit_readings;
    double *readings;

    unit_readings = NULL;
    readings = NULL;
    int rc =
    wl->workload_func(wl->total_work_amount, &pilot_malloc_func,
                      &num_of_work_units, &unit_readings,
                      &readings);
    // result check first
    if (0 != rc)   return 12;
    if (!readings) return 13;

    // move all data into the permanent location
    for (int piid = 0; piid < wl->num_of_pi; ++piid)
        wl->readings[piid].push_back(readings[piid]);
    // it is ok for unit_readings to be NULL
    if (unit_readings) {
        info_log << "got num_of_work_units = " << num_of_work_units;
        for (int piid = 0; piid < wl->num_of_pi; ++piid) {
            wl->unit_readings[piid].emplace_back(vector<double>(unit_readings[piid], unit_readings[piid] + num_of_work_units));
            free(unit_readings[piid]);
        }
    } else {
        //! TODO: need to handle empty unit_readings?
    }

    //! TODO: save the data to a database

    // clean up
    if (readings) free(readings);
    if (unit_readings) free(unit_readings);
    ++wl->rounds;
    return 0;
}

const char *pilot_strerror(int errnum) {
    switch (errnum) {
    case 0:   return "No error";
    case 11:  return "Workload not properly initialized";
    case 12:  return "Workload failure";
    case 13:  return "Workload did not return readings";
    case 200: return "Not implemented";
    default:  return "Unknown error code";
    }
}

int pilot_export_session_data(pilot_workload_t *wl, const char *file_name) {
    /*! \todo implement function */
    abort();
    return 200;
}

void* pilot_malloc_func(size_t size) {
    return malloc(size);
}

int pilot_get_num_of_rounds(const pilot_workload_t *wl) {
    if (!wl) {
        error_log << "wl is NULL";
        return -1;
    }
    return wl->rounds;
}

const double* pilot_get_pi_readings(const pilot_workload_t *wl, size_t piid) {
    if (!wl) {
        error_log << "wl is NULL";
        return NULL;
    }
    if (piid >= wl->num_of_pi) {
        error_log << "piid out of range";
        return NULL;
    }
    return wl->readings[piid].data();
}

const double* pilot_get_pi_unit_readings(const pilot_workload_t *wl,
    size_t piid, size_t round, size_t *num_of_work_units) {
    if (!wl) {
        error_log << "wl is NULL";
        return NULL;
    }
    if (piid >= wl->num_of_pi) {
        error_log << "piid out of range";
        return NULL;
    }
    if (round >= wl->rounds) {
        error_log << "round out of range";
        return NULL;
    }
    *num_of_work_units = wl->unit_readings[piid][round].size();
    return wl->unit_readings[piid][round].data();
}

int pilot_destroy_workload(pilot_workload_t *wl) {
    if (!wl) {
        error_log << "wl is NULL";
        return 11;
    }
    delete wl;
    return 0;
}
