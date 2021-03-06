/*
 * libpilot.h: the main header file of libpilot. This is the only header file
 * you need to include.
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

#ifndef LIBPILOT_HEADER_LIBPILOT_H_
#define LIBPILOT_HEADER_LIBPILOT_H_

// This file must retain C99-compatibility so do not include C++ header files
// here.
#include <limits.h>
#include "config.h"
#include <fcntl.h>
#include "pilot_exports.h"
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
namespace pilot {
#endif

#ifdef __cplusplus
    #define DEFAULT_VALUE(value) = value
#else
    #define DEFAULT_VALUE(value)
#endif

#ifdef __cplusplus
    #define NOEXCEPT noexcept
#else
    #define NOEXCEPT
#endif

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllexport))
    #else
      #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllimport))
    #else
      #define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DLL_PUBLIC
    #define DLL_LOCAL
  #endif
#endif

enum pilot_error_t {
    NO_ERROR = 0,
    ERR_WRONG_PARAM = 2,
    ERR_NOMEM = 3,
    ERR_IO = 5,
    ERR_UNKNOWN_HOOK = 6,
    ERR_NOT_INIT = 11,
    ERR_WL_FAIL = 12,
    ERR_STOPPED_BY_DURATION_LIMIT = 13,
    ERR_STOPPED_BY_HOOK = 14,
    ERR_STOPPED_BY_REQUEST = 15,
    ERR_TOO_MANY_REJECTED_ROUNDS = 20,
    ERR_NOT_ENOUGH_DATA = 30,
    ERR_NOT_ENOUGH_DATA_FOR_CI = 31,
    ERR_NO_DOMINANT_SEGMENT = 32,
    ERR_ROUND_TOO_SHORT = 33,
    ERR_NO_CHANGEPOINT = 34,
    ERR_NOT_IMPL = 200,
    ERR_LINKED_WRONG_VER = 201
};

enum pilot_mean_method_t {
    ARITHMETIC_MEAN = 0,
    HARMONIC_MEAN = 1
};

enum pilot_reading_type_t {
    READING_TYPE = 0,
    UNIT_READING_TYPE = 1,
    WPS_TYPE = 2
};

enum pilot_confidence_interval_type_t {
    SAMPLE_MEAN = 0,         /**< Confidence interval of sample mean */
    BINOMIAL_PROPORTION = 1  /**< Confidence interval of binomial proportion */
};

typedef int_least64_t nanosecond_type;

/**
 * \brief Performance libpilot self check and initialization
 * \details Include this macro at the beginning of your program.
 */
#define PILOT_LIB_SELF_CHECK pilot::pilot_lib_self_check(PILOT_VERSION_MAJOR, \
        PILOT_VERSION_MINOR, sizeof(pilot::nanosecond_type))

DLL_PUBLIC void pilot_lib_self_check(int vmajor, int vminor, size_t nanosecond_type_size) NOEXCEPT;

DLL_PUBLIC void pilot_remove_console_log_sink(void) NOEXCEPT;

/**
 * \brief The type of memory allocation function, which is used by pilot_workload_func_t.
 * \details libpilot needs to ask the caller to allocate some memory and write
 * in the result readings. The caller need to use this function to allocate memory
 * that will be owned by libpilot.
 * @param size the size of memory to allocate
 */
typedef void* pilot_malloc_func_t(size_t size);
DLL_PUBLIC void* pilot_malloc_func(size_t size) NOEXCEPT;

/**
 * \brief Free memory allocated by a pilot_malloc_func_t
 */
DLL_PUBLIC void pilot_free(void *p) NOEXCEPT;

/**
 * \brief The data object that holds all the information of a workload.
 * \details Use pilot_new_workload to allocate an instance. The library user
 * should never be able to directly declare such a struct.
 */
struct pilot_workload_t;

/**
 * \brief A function from the library's user for running one benchmark and collecting readings.
 * @param[in] total_work_amount
 * @param[in] data arbitrary data that can be passed to the workload func. Set by using pilot_set_workload_data().
 * @param[out] num_of_work_unit
 * @param[out] unit_readings the reading of each work unit. Format: unit_readings[piid][unit_id]. The user needs to allocate memory using lib_malloc_func.
 * @param[out] readings the final readings of this workload run. Format: readings[piid]. The user needs to allocate memory using lib_malloc_func.
 * @return
 */
typedef int pilot_workload_func_t(const pilot_workload_t *wl,
                                  size_t round,
                                  size_t total_work_amount,
                                  pilot_malloc_func_t *lib_malloc_func,
                                  size_t *num_of_work_unit,
                                  double ***unit_readings,
                                  double **readings,
                                  nanosecond_type *round_duration,
                                  void *data);

/**
 * \brief A simplified version of pilot_workload_func_t that supports setting
 * work amount.
 * @param[in] round_work_amount
 * @return 0 on success; any other values stop the benchmark
 */
typedef int pilot_simple_workload_with_wa_func_t(size_t round_work_amount);

/**
 * \brief A simplified version of pilot_workload_func_t that doesn't support
 * setting work amount.
 * @return 0 on success; any other values stop the benchmark
 */
typedef int pilot_simple_workload_func_t(void);

struct pilot_pi_unit_readings_iter_t;

/**
 * \brief the function that calculates how many samples are needed for getting
 * the desired result (used mainly in test cases)
 * \details this function calculates the number of samples needed from the
 * existing readings and the desired confidence interval and confidence
 * level as specified in wl
 * @param[in] wl pointer to the workload struct
 * @param piid the Performance Index to calculate
 * @return
 */
typedef ssize_t calc_required_readings_func_t(const pilot_workload_t* wl, int piid);

DLL_PUBLIC void pilot_set_calc_required_readings_func(pilot_workload_t* wl, calc_required_readings_func_t *f) NOEXCEPT;

/**
 * \brief Set the function hook that calculates how many unit readings are needed
 * for a session (used mainly in test cases)
 * @param[in] wl pointer to the workload struct
 * @param f the new hook function
 */
DLL_PUBLIC void pilot_set_calc_required_unit_readings_func(pilot_workload_t* wl, calc_required_readings_func_t *f) NOEXCEPT;

/**
 * \brief a function that returns what work amount should be used for next
 * round (used mainly in test cases)
 * @param[in] wl pointer to the workload struct
 * @param[out] needed_work_amount. A returned 0 value doesn't necessarily mean
 * no more work is needed, because the workload may not support setting work
 * amount. Always use the return value to decide if more rounds are needed.
 * If return value is false (no more round is needed) this value can still be
 * useful for further refining the result.
 * @return true if more rounds are needed; false if no more rounds are needed
 */
typedef bool next_round_work_amount_hook_t(const pilot_workload_t* wl, size_t *needed_work_amount);

/**
 * \brief Set the function hook that calculates how many readings are needed
 * for a session (used mainly in test cases)
 * @param[in] wl pointer to the workload struct
 * @param f the new hook function
 */
void pilot_set_next_round_work_amount_hook(pilot_workload_t* wl, next_round_work_amount_hook_t *f) NOEXCEPT;

/**
 * \brief Type for the general hook functions
 * @param[in] wl pointer to the workload struct
 * @return return false to stop the execution of the ongoing libpilot process
 */
typedef bool general_hook_func_t(pilot_workload_t* wl);

enum pilot_hook_t {
    PRE_WORKLOAD_RUN,
    POST_WORKLOAD_RUN
};

/**
 * \brief Set a hook function
 * @param[in] wl pointer to the workload struct
 * @param hook the hook to change
 * @param f the new hook function
  * @return 0 on success; otherwise error code
 */
DLL_PUBLIC int pilot_set_hook_func(pilot_workload_t* wl, enum pilot_hook_t hook, general_hook_func_t *f) NOEXCEPT;

/**
 * \brief Type for a function that formats a number for output
 * \details There are several hooks of this type used by libpilots to
 * format a number before rendering it for display. You can set a
 * different function for each PI.
 * @param[in] wl pointer to the workload struct
 * @param number the number to be displayed
 * @return a processed number for print
 */
typedef double pilot_pi_display_format_func_t(const pilot_workload_t* wl, double number);

/**
 * \brief Set the information of a PI
 * @param[in] wl pointer to the workload struct
 * @param piid the piid of the PI to change
 * @param[in] pi_name the name of the PI
 * @param[in] pi_unit the unit of the for displaying a reading, like "MB/s"
 * @param[in] reading_display_preprocess_func the function for preprocessing a
 * reading number for display. Setting this parameter to NULL to disable it.
 * @param[in] unit_reading_display_preprocess_func the function for preprocessing a
 * unit reading number for display. Setting this parameter to NULL to disable
 * it.
 * @param reading_must_satisfy set if the reading must satisfy quality
 * requirements
 * @param unit_reading_must_satisfy set if the unit readings must satisfy
 * quality requirements
 */
DLL_PUBLIC void pilot_set_pi_info(pilot_workload_t* wl, int piid,
        const char *pi_name,
        const char *pi_unit DEFAULT_VALUE(NULL),
        pilot_pi_display_format_func_t *format_reading_func DEFAULT_VALUE(NULL),
        pilot_pi_display_format_func_t *format_unit_reading_func DEFAULT_VALUE(NULL),
        bool reading_must_satisfy DEFAULT_VALUE(false), bool unit_reading_must_satisfy DEFAULT_VALUE(false),
        pilot_mean_method_t reading_mean_type DEFAULT_VALUE(ARITHMETIC_MEAN),
        pilot_mean_method_t unit_reading_mean_type DEFAULT_VALUE(ARITHMETIC_MEAN),
        pilot_confidence_interval_type_t reading_ci_type DEFAULT_VALUE(SAMPLE_MEAN)) NOEXCEPT;

// TODO: implement a get_pi_info()

DLL_PUBLIC pilot_workload_t* pilot_new_workload(const char *workload_name) NOEXCEPT;

/**
 * Set an arbitrary pointer that will be passed to the workload_func
 * @param data
 */
DLL_PUBLIC void pilot_set_workload_data(pilot_workload_t* wl, void *data) NOEXCEPT;

/**
 * \brief Set the number of performance indices to record
 * \details You should set the number of PIs right after creating a new workload.
 * Calling this function when there are already stored workload data is NOT
 * supported.
 * @param[in] wl pointer to the workload struct
 * @param num_of_pi the number of performance indices
 */
DLL_PUBLIC void pilot_set_num_of_pi(pilot_workload_t* wl, size_t num_of_pi) NOEXCEPT;

/**
 * \brief Get the number of performance indices
 * @param[in] wl pointer to the workload struct
 * @param[out] p_num_of_pi the pointer for storing the num_of_pi
 * @return 0 on success; aborts if wl is NULL; otherwise error code
 */
DLL_PUBLIC int pilot_get_num_of_pi(const pilot_workload_t* wl, size_t *p_num_of_pi) NOEXCEPT;

DLL_PUBLIC void pilot_set_workload_func(pilot_workload_t*, pilot_workload_func_t*) NOEXCEPT;

/**
 * \brief Set the upper limit for work amount that pilot should attempt
 * \details pilot will start with this init_work_amount and repeat the workload
 * and/or increase the work amount until the required confidence level
 * can be achieved, but it will never do more than work_amount_limit
 * in a single round. Set it to 0 if the workload doesn't need a work amount.
 * @param[in] wl pointer to the workload struct
 * @param init_work_amount the initial work amount
 */
DLL_PUBLIC void pilot_set_work_amount_limit(pilot_workload_t* wl, size_t work_amount_limit) NOEXCEPT;

/**
 * \brief Get work_amount_limit, the upper limit of work amount that pilot should attempt
 * @param[in] wl pointer to the workload struct
 * @param[out] p_work_amount_limit the pointer for storing the work_amount_limit
 * @return 0 on success; aborts if wl is NULL; otherwise error code
 */
DLL_PUBLIC int pilot_get_work_amount_limit(const pilot_workload_t* wl, size_t *p_work_amount_limit) NOEXCEPT;

/**
 * \brief Set the initial work amount that pilot should attempt
 * \details pilot will start with this init_work_amount and repeat the workload
 * and/or increase the work amount until the required confidence level
 * can be achieved, but it will never do more than work_amount_limit
 * in a single round.
 * @param[in] wl pointer to the workload struct
 * @param init_work_amount the initial work amount
 */
DLL_PUBLIC void pilot_set_init_work_amount(pilot_workload_t* wl, size_t init_work_amount) NOEXCEPT;

/**
 * \brief Get init_work_amount, the initial work amount that pilot should attempt
 * @param[in] wl pointer to the workload struct
 * @param[out] p_init_work_amount the pointer for storing init_work_amount
 * @return 0 on success; aborts if wl is NULL; otherwise error code
 */
DLL_PUBLIC int pilot_get_init_work_amount(const pilot_workload_t* wl, size_t *p_init_work_amount) NOEXCEPT;

enum pilot_warm_up_removal_detection_method_t {
    NO_WARM_UP_REMOVAL = 0,
    FIXED_PERCENTAGE,
    EDM,
};

/**
 * \brief Set the warm-up removal method
 * @param[in] wl pointer to the workload struct
 * @param m the warm-up removal method
 */
DLL_PUBLIC void pilot_set_warm_up_removal_method(pilot_workload_t* wl, pilot_warm_up_removal_detection_method_t m) NOEXCEPT;

/**
 * \brief Set the percentage to remove as warm-up
 * If the warm-up removal method is not FIXED_PERCENTAGE, this value set here
 * has no effect on anything.
 * @param[in] wl pointer to the workload struct
 * @param percent the percent to remove
 */
DLL_PUBLIC void pilot_set_warm_up_removal_percentage(pilot_workload_t* wl, double percent) NOEXCEPT;

/**
 * \brief Detect the ending location of the warm-up phase
 * @param[in] data input data
 * @param n size of input data
 * @param round_duration the duration of the round
 * @param method the detection method
 * @return 0 on success; otherwise error code
 */
DLL_PUBLIC int pilot_warm_up_removal_detect(const pilot_workload_t *wl,
                                 const double *data,
                                 size_t n,
                                 nanosecond_type round_duration,
                                 pilot_warm_up_removal_detection_method_t method,
                                 size_t *begin, size_t *end) NOEXCEPT;

/**
 * \brief Whether to check for very short-lived workload
 * @param[in] wl pointer to the workload struct
 * @param check_short_workload true to enable short-lived workload check
 */
DLL_PUBLIC void pilot_set_short_workload_check(pilot_workload_t* wl, bool check_short_workload) NOEXCEPT;

/**
 * \brief Run the workload as specified in wl
 * @param[in] wl pointer to the workload struct
 * @return 0 on success, otherwise error code. On error, call pilot_strerror to
 * get a pointer to the error message.
 */
DLL_PUBLIC int pilot_run_workload(pilot_workload_t *wl) NOEXCEPT;

/**
 * \brief Run the workload as specified in wl using the text user interface
 * \details Call pilot_set_pi_info() prior to running TUI to set up the PI information that
 * will be used in the TUI display.
 * @param[in] wl pointer to the workload struct
 * @return 0 on success, otherwise error code. On error, call pilot_strerror to
 * get a pointer to the error message.
 */
DLL_PUBLIC int pilot_run_workload_tui(pilot_workload_t *wl) NOEXCEPT;

/**
 * \brief Request to stop a workload at a proper time
 * \details This function is usually used in signal handlers.
 * @param[in] wl pointer to the workload struct
 */
DLL_PUBLIC void pilot_stop_workload(pilot_workload_t *wl) NOEXCEPT;

/**
 * \brief Print a message into the UI's message box
 * @param[in] wl pointer to the workload struct
 * @param format a format string using the same format as plain printf
 */
DLL_PUBLIC void pilot_ui_printf(pilot_workload_t *wl, const char* format, ...) NOEXCEPT;

/**
 * \brief Print a message into the UI's message box using highlight font
 * @param[in] wl pointer to the workload struct
 * @param format a format string using the same format as plain printf
 */
DLL_PUBLIC void pilot_ui_printf_hl(pilot_workload_t *wl, const char* format, ...) NOEXCEPT;

/**
 * \brief Get the number of total valid unit readings after warm-up removal
 * @param[in] wl pointer to the workload struct
 * @return the total number of valid unit readings
 */
DLL_PUBLIC size_t pilot_get_total_num_of_unit_readings(const pilot_workload_t *wl, int piid) NOEXCEPT;

/**
 * \brief Get pointer to error message string
 * @param errnum the error number returned by a libpilot function
 * @return a pointer to a static memory of error message
 */
DLL_PUBLIC const char *pilot_strerror(int errnum) NOEXCEPT;

/**
 * \brief Estimate the sample variance when sample cannot be proven not
 * correlated and the distribution of sample mean is unknown.
 * \details This function uses the method of independent replications as
 * described in equation (2.18) and (2.19) in [Ferrari78].
 * @param n sample size
 * @param sample sample data
 * @param q size of independent subsessions
 * @return
 */
DLL_PUBLIC int pilot_est_sample_var_dist_unknown(const size_t n, const double *sample, size_t q) NOEXCEPT;

/**
 * \brief Return the total number of rounds so far.
 * @param[in] wl pointer to the workload struct
 * @return the number of rounds; a negative number on error
 */
DLL_PUBLIC int pilot_get_num_of_rounds(const pilot_workload_t *wl) NOEXCEPT;

/**
 * \brief Return the read only copy of all raw readings of a performance index
 * @param[in] wl pointer to the workload struct
 * @param piid Performance Index ID
 * @return a pointer to readings data, the length of which can be get by using pilot_get_num_of_rounds() NOEXCEPT; NULL on error.
 */
DLL_PUBLIC const double* pilot_get_pi_readings(const pilot_workload_t *wl, size_t piid) NOEXCEPT;

/**
 * \brief Return the read only copy of all raw unit readings of a performance index in a certain round
 * @param[in] wl pointer to the workload struct
 * @param piid Performance Index ID
 * @param round Performance Index ID
 * @param[out] num_of_work_units the number of work units in that round
 * @return the data of all unit readings of PIID in that round. It is a read-only array of size num_of_work_units.
 */
DLL_PUBLIC const double* pilot_get_pi_unit_readings(const pilot_workload_t *wl, size_t piid, size_t round, size_t *num_of_work_units) NOEXCEPT;

/**
 * \brief Export workload data
 * \details Multiple files will be created in a directory.
 * @param[in] wl pointer to the workload struct
 * @param[in] dirname the directory to store the exported files. It will be
 * created if needed.
 * @return 0 on success; aborts if wl is NULL; otherwise returns an error code. On error, call pilot_strerror to
 * get a pointer to the error message.
 */
DLL_PUBLIC int pilot_export(const pilot_workload_t *wl, const char *dirname) NOEXCEPT;

/**
 * \brief Destroy (free) a workload struct
 * @param[in] wl pointer to the workload struct
 * @return 0 on success; aborts if wl is NULL; otherwise returns an error code. On error, call pilot_strerror to
 * get a pointer to the error message.
 */
DLL_PUBLIC int pilot_destroy_workload(pilot_workload_t *wl) NOEXCEPT;

enum pilot_log_level_t
{
    lv_trace,
    lv_debug,
    lv_info,
    lv_warning,
    lv_error,
    lv_fatal,
    lv_no_show,   /* Set log level to this to prevent all logs from being shown */
};
/**
 * \brief Set the logging level of the library
 * @param log_level
 */
DLL_PUBLIC void pilot_set_log_level(pilot_log_level_t log_level) NOEXCEPT;

/**
 * \brief Get last n log lines
 * @return A pointer to the static log lines. Need to be freed using pilot_free()
 * @param n lines of log to get
 */
DLL_PUBLIC const char* pilot_get_last_log_lines(size_t n DEFAULT_VALUE(1)) NOEXCEPT;

/**
 * \brief Get the logging level of the library
 * @return log_level
 */
pilot_log_level_t pilot_get_log_level(void) NOEXCEPT;

DLL_PUBLIC double pilot_subsession_mean_p(const double *data, size_t n, pilot_mean_method_t mean_method) NOEXCEPT;

/**
 * \brief Calculate the subsession covariance of data
 * \details n/q must be greater than or equal 2
 * @param[in] data the input data
 * @param n the number of samples of data
 * @param q the subsession size
 * @param sample_mean the sample mean
 * @return the calculated covariance
 */
DLL_PUBLIC double pilot_subsession_auto_cov_p(const double *data, size_t n, size_t q, double sample_mean, pilot_mean_method_t mean_method) NOEXCEPT;

DLL_PUBLIC double pilot_subsession_var_p(const double *data, size_t n, size_t q, double sample_mean, pilot_mean_method_t mean_method) NOEXCEPT;
DLL_PUBLIC double pilot_subsession_autocorrelation_coefficient_p(const double *data, size_t n, size_t q, double sample_mean, pilot_mean_method_t mean_method) NOEXCEPT;

/**
 * \brief Calculate the mean and confidence interval of WPS with warm-up
 * removal using the linear regression method
 * \details This function is useful for test cases that cannot provide unit
 * reading. Note that we encourage using CI instead of just a mean because
 * you should not guarantee that the CI is symmetrical. If you really just
 * need one number as a mean, you can use (ci_right - ci_left)/2 + ci_left
 * as the mean if the CI is not too wide.
 * @param rounds total number of rounds
 * @param[in] round_work_amounts the work amounts of each round
 * @param[in] round_durations the duration of each round
 * @param autocorrelation_coefficient_limit the limit for autocorrelation
 * coefficient (usually 0.1)
 * @param[out] v the calculated performance
 * @param[out] ci_width the calculated width of the confidence interval
 * @return 0 on success; ERR_NOT_ENOUGH_DATA when there is not enough sample
 * for calculate v; ERR_NOT_ENOUGH_DATA_FOR_CI when there is enough data for
 * calculating v but not enough for calculating confidence interval.
 */
DLL_PUBLIC int pilot_wps_warmup_removal_lr_method_p(size_t rounds, const size_t *round_work_amounts,
        const nanosecond_type *round_durations,
        float autocorrelation_coefficient_limit,
        nanosecond_type duration_threshold,
        double *alpha, double *v,
        double *ci_width, double *ssr_out DEFAULT_VALUE(NULL), double *ssr_out_percent DEFAULT_VALUE(NULL),
        size_t *subsession_sample_size DEFAULT_VALUE(NULL)) NOEXCEPT;

/**
 * \brief Basic and statistics information of a workload round
 */
#pragma pack(push, 1)
struct pilot_round_info_t {
    size_t work_amount;
    nanosecond_type round_duration;
    size_t* num_of_unit_readings;
    size_t* warm_up_phase_lens;
};
#pragma pack(pop)

/**
 * \brief Get the basic and statistics information of a workload round
 * \details If info is NULL, this function allocates memory for a
 * pilot_round_info_t, fills information, and returns it. If
 * info is provided, its information will be updated and no new
 * memory will be allocated. The allocated memory must eventually
 * be freed by using pilot_free_round_info().
 * @param[in] wl pointer to the workload struct
 * @param info (optional) if provided, it must point to a pilot_round_info_t
 * that was returned by a previous call to pilot_round_info()
 * @return a pointer to a pilot_round_info_t struct
 */
DLL_PUBLIC pilot_round_info_t* pilot_round_info(const pilot_workload_t *wl, size_t round, pilot_round_info_t *info DEFAULT_VALUE(NULL)) NOEXCEPT;

/**
 * \brief Basic and statistics information of a workload
 */
#pragma pack(push, 1)
struct pilot_analytical_result_t {
    size_t  num_of_pi;
    size_t  num_of_rounds;
    double  session_duration;          //! the total session duration so far
    // Readings analysis
    size_t* readings_num;              //! the following readings fields are undefined if readings_num is 0
    pilot_mean_method_t* readings_mean_method;
    pilot_confidence_interval_type_t* readings_ci_type;
    size_t* readings_last_changepoint;

    // Dominant segment analysis (these info. are preferred to raw data)
    double* readings_mean;             //! the mean of all readings so far according to PI reading's mean method; is undefined if readings_num < 2
    double* readings_mean_formatted;   //! the mean after being formatted by format_reading() NOEXCEPT; is undefined if readings_num < 2
    double* readings_var;              //! is undefined if readings_num < 2
    double* readings_var_formatted;    //! is undefined if readings_num < 2
    double* readings_autocorrelation_coefficient; //! is undefined if readings_num < 2
    ssize_t* readings_required_sample_size;  //! is -1 if not enough data
    ssize_t* readings_optimal_subsession_size; //! is undefined if readings_required_sample_size < 0
    double* readings_optimal_subsession_var; //! is undefined if readings_required_sample_size < 0
    double* readings_optimal_subsession_var_formatted; //! is undefined if readings_required_sample_size < 0
    double* readings_optimal_subsession_autocorrelation_coefficient; //! is undefined if readings_required_sample_size < 0
    double* readings_optimal_subsession_ci_width; //! is undefined if readings_required_sample_size < 0
    double* readings_optimal_subsession_ci_width_formatted; //! is undefined if readings_required_sample_size < 0

    // Raw data uses all values
    double* readings_raw_mean;             //! the mean of all readings so far according to PI reading's mean method
    double* readings_raw_mean_formatted;   //! the mean after being formatted by format_reading()
    double* readings_raw_var;
    double* readings_raw_var_formatted;
    double* readings_raw_autocorrelation_coefficient;
    ssize_t* readings_raw_required_sample_size; //! is -1 if not enough data
    ssize_t* readings_raw_optimal_subsession_size; //! is undefined if readings_raw_required_sample_size < 0
    double* readings_raw_optimal_subsession_var; //! is undefined if readings_raw_required_sample_size < 0
    double* readings_raw_optimal_subsession_var_formatted; //! is undefined if readings_raw_required_sample_size < 0
    double* readings_raw_optimal_subsession_autocorrelation_coefficient; //! is undefined if readings_raw_required_sample_size < 0
    double* readings_raw_optimal_subsession_ci_width; //! is undefined if readings_raw_required_sample_size < 0
    double* readings_raw_optimal_subsession_ci_width_formatted; //! is undefined if readings_raw_required_sample_size < 0

    // Unit-readings analysis
    size_t* unit_readings_num;
    double* unit_readings_mean;
    double* unit_readings_mean_formatted;
    pilot_mean_method_t* unit_readings_mean_method;
    double* unit_readings_var;
    double* unit_readings_var_formatted;
    double* unit_readings_autocorrelation_coefficient;
    ssize_t* unit_readings_optimal_subsession_size; //! is -1 if not enough data
    double* unit_readings_optimal_subsession_var; //! is undefined if unit_readings_optimal_subsession_size < 0
    double* unit_readings_optimal_subsession_var_formatted; //! is undefined if unit_readings_optimal_subsession_size < 0
    double* unit_readings_optimal_subsession_autocorrelation_coefficient; //! is undefined if unit_readings_optimal_subsession_size < 0
    double* unit_readings_optimal_subsession_ci_width; //! is undefined if unit_readings_optimal_subsession_size < 0
    double* unit_readings_optimal_subsession_ci_width_formatted; //! is undefined if unit_readings_optimal_subsession_size < 0
    ssize_t* unit_readings_required_sample_size; //! is -1 if not enough data
    int*    unit_readings_required_sample_size_is_from_user; //! Whether unit_readings_required_sample_size is from the calc_required_unit_readings_func

    // Work amount-per-second analysis
    bool   wps_has_data;               //! whether the following fields have data
    size_t wps_subsession_sample_size; //! sample size after merging adjacent samples to reduce autocorrelation coefficient
    double wps_harmonic_mean;          //! wps is a rate so only harmonic mean is valid
    double wps_harmonic_mean_formatted;
    double wps_naive_v_err;
    double wps_naive_v_err_percent;
    double wps_alpha;                  //! the alpha as in t = alpha + v*w
    double wps_v;                      //! the v as in t = alpha + v*w
    double wps_v_formatted;
    size_t wps_optimal_subsession_size;
    double wps_err;
    double wps_err_percent;
    double wps_v_ci;                   //! the width of the confidence interval of v
    double wps_v_ci_formatted;

#ifdef __cplusplus
    inline void _free_all_field();
    inline void _copyfrom(const pilot_analytical_result_t &a);
    pilot_analytical_result_t();
    pilot_analytical_result_t(const pilot_analytical_result_t &a);
    ~pilot_analytical_result_t();
    void set_num_of_pi(size_t new_num_of_pi);
    pilot_analytical_result_t& operator=(const pilot_analytical_result_t &a);
#endif
};
#pragma pack(pop)

/**
 * \brief Get the basic and statistics information of a workload
 * \details If info is NULL, this function allocates memory for a
 * pilot_workload_info_t, fills information, and returns it. If
 * info is provided, its information will be updated and no new
 * memory will be allocated. The allocated memory must eventually
 * be freed by using pilot_free_workload_info().
 * @param[in] wl pointer to the workload struct
 * @param info (optional) if provided, it must point to a pilot_workload_info_t
 * that was returned by a previous call to pilot_analytical_result()
 * @return a pointer to a pilot_analytical_result_t struct
 */
DLL_PUBLIC pilot_analytical_result_t* pilot_analytical_result(const pilot_workload_t *wl, pilot_analytical_result_t *info DEFAULT_VALUE(NULL)) NOEXCEPT;

DLL_PUBLIC void pilot_free_analytical_result(pilot_analytical_result_t *info) NOEXCEPT;

DLL_PUBLIC void pilot_free_round_info(pilot_round_info_t *info) NOEXCEPT;

/**
 * Dump the workload summary in Markdown text format
 * @param[in] wl pointer to the workload struct
 * @return a memory buffer of text dump that can be directly output.
 * Use pilot_free_text_dump() to free the buffer after using.
 */
DLL_PUBLIC char* pilot_text_workload_summary(const pilot_workload_t *wl) NOEXCEPT;

/**
 * Dump the summary of a round in Markdown text format
 * @param[in] wl pointer to the workload struct
 * @param round_id the round ID starting from 0
 * @return a memory buffer of text dump that can be directly output.
 * Use pilot_free_text_dump() to free the buffer after using.
 */
DLL_PUBLIC char* pilot_text_round_summary(const pilot_workload_t *wl, size_t round_id) NOEXCEPT;

DLL_PUBLIC void pilot_free_text_dump(char *dump) NOEXCEPT;

/**
 * \brief Calculate the optimal subsession size (q) so that autocorrelation coefficient doesn't exceed the limit
 * @param[in] data the input data
 * @param n size of the input data
 * @param max_autocorrelation_coefficient the maximal limit of the autocorrelation coefficient
 * @return the size of subsession (q) NOEXCEPT; -1 if q can't be found (e.g. q would be larger than n)
 */
DLL_PUBLIC int pilot_optimal_subsession_size_p(const double *data, size_t n,
        pilot_mean_method_t mean_method, double max_autocorrelation_coefficient DEFAULT_VALUE(0.1)) NOEXCEPT;

/**
 * \brief Calculate the width of the confidence interval given subsession size q and confidence level
 * @param[in] data the input data
 * @param n size of the input data
 * @param q size of each subsession
 * @param confidence_level the probability that the real mean falls within the confidence interval, e.g., .95
 * @return the width of the confidence interval
 */
DLL_PUBLIC double pilot_subsession_confidence_interval_p(const double *data, size_t n, size_t q,
                                                         double confidence_level,
                                                         pilot_mean_method_t mean_method,
                                                         pilot_confidence_interval_type_t ci_type DEFAULT_VALUE(SAMPLE_MEAN)) NOEXCEPT;

/**
 * \brief Calculate the degree of freedom using Welch-Satterthwaite equation
 * @param var1
 * @param var2
 * @param size1
 * @param size2
 * @return the degree of freedom
 */
DLL_PUBLIC double __attribute__ ((const)) pilot_calc_deg_of_freedom(double var1, double var2, size_t size1, size_t size2) NOEXCEPT;

/**
 * \brief Calculate the p-value for the hypothesis mean1 == mean2
 * @param mean1
 * @param mean2
 * @param size1
 * @param size2
 * @param var1
 * @param var2
 * @param[out] ci_left
 * @param[out] ci_right
 * @param confidence_level
 * @return the p-value
 */
DLL_PUBLIC double pilot_p_eq(double mean1, double mean2, size_t size1, size_t size2,
                  double var1, double var2, double *ci_left, double *ci_right,
                  double confidence_level DEFAULT_VALUE(0.95)) NOEXCEPT;

/**
 * \brief Calculate the sample size needed for comparing against a baseline
 * to achieve a certain p-value
 * @param baseline_mean
 * @param baseline_sample_size
 * @param baseline_var
 * @param new_mean
 * @param new_sample_size
 * @param new_var
 * @param required_p
 * @param[out] opt_new_sample_size the optimal sample size needed for new
 * @return 0 on success; error code otherwise
 */
DLL_PUBLIC int pilot_optimal_sample_size_for_eq_test(double baseline_mean,
        size_t baseline_sample_size, double baseline_var,
        double new_mean, size_t new_sample_size, double new_var,
        double required_p, size_t *opt_new_sample_size) NOEXCEPT;

/**
 * \brief Calculate the optimal length of the benchmark session given observed
 *        data, confidence interval, confidence level, and maximal
 *        autocorrelation coefficient. This function uses a pointer as data
 *        source.
 * @param[in] data the input data
 * @param n size of the input data
 * @param confidence_interval_width
 * @param mean_method the method for calculating means
 * @param[out] q the desired subsession size
 * @param[out] opt_sample_size the optimal subsession sample size
 * @param confidence_level
 * @param max_autocorrelation_coefficient
 * @return true if the calculation was successful; false if there was not
 * enough data for calculation
 */
DLL_PUBLIC bool
pilot_optimal_sample_size_p(const double *data, size_t n,
                            double confidence_interval_width,
                            pilot_mean_method_t mean_method,
                            size_t *q, size_t *opt_sample_size,
                            pilot_confidence_interval_type_t ci_type DEFAULT_VALUE(SAMPLE_MEAN),
                            double confidence_level DEFAULT_VALUE(0.95),
                            double max_autocorrelation_coefficient DEFAULT_VALUE(0.1)) NOEXCEPT;

#define MIN_CHANGEPOINT_DETECTION_SAMPLE_SIZE (30)

/**
 * \brief Detect changepoint of mean in data
 * Use pilot_free() to free the memory you get in changepoints
 * @param[in] data input data
 * @param n size of input data
 * @param[out] changepoints the detected points
 * @param[out] cp_n the number of changepoints
 * @return 0 on success; otherwise error code
 */
DLL_PUBLIC int pilot_changepoint_detection(const double *data, size_t n,
        int **changepoints, size_t *cp_n, double percent DEFAULT_VALUE(0.25),
        int degree DEFAULT_VALUE(1)) NOEXCEPT;

/**
 * Find the dominant segment
 * \brief The dominant segment is the longest segment between changepoints. It
 * has to contain more than 50\% of total samples.
 * @param data
 * @param n
 * @param min_size
 * @param percent
 * @param degree
 * @param begin
 * @param end
 * @return 0 on success, otherwise error code
 */
DLL_PUBLIC int pilot_find_dominant_segment(const double *data, size_t n, size_t *begin,
        size_t *end, size_t min_size DEFAULT_VALUE(MIN_CHANGEPOINT_DETECTION_SAMPLE_SIZE),
        double percent DEFAULT_VALUE(0.25), int degree DEFAULT_VALUE(1)) NOEXCEPT;

/**
 * Use EDM tail method to find one changepoint
 * @param data
 * @param n
 * @param [out] loc for storing the detected changepoint
 * @return 0 on success, otherwise error code
 */
DLL_PUBLIC int pilot_find_one_changepoint(const double *data, size_t n, size_t *loc,
                               double percent DEFAULT_VALUE(0.25), int degree DEFAULT_VALUE(1)) NOEXCEPT;

struct pilot_pi_unit_readings_iter_t;

/**
 * \brief Get a forward iterator for going through the unit readings of a PI
 * with warm-up phase removed
 * @param[in] wl pointer to the workload struct
 * @param piid the ID of the PI
 * @return a iterator
 */
DLL_PUBLIC pilot_pi_unit_readings_iter_t*
pilot_pi_unit_readings_iter_new(const pilot_workload_t *wl, int piid) NOEXCEPT;

/**
 * \brief Get the value pointed to by the iterator
 * @param[in] iter a iterator get by using pilot_get_pi_unit_readings_iter()
 * @return the value pointed to by the iterator
 */
DLL_PUBLIC double pilot_pi_unit_readings_iter_get_val(const pilot_pi_unit_readings_iter_t* iter) NOEXCEPT;

/**
 * \brief Move the iterator to next value and return it
 * @param[in] iter a iterator get by using pilot_get_pi_unit_readings_iter()
 */
DLL_PUBLIC void pilot_pi_unit_readings_iter_next(pilot_pi_unit_readings_iter_t* iter) NOEXCEPT;

/**
 * \brief Check if the iterator points to a valid reading
 * @param[in] iter a iterator get by using pilot_get_pi_unit_readings_iter()
 * @return true on yes; false on end of data
 */
DLL_PUBLIC bool pilot_pi_unit_readings_iter_valid(const pilot_pi_unit_readings_iter_t* iter) NOEXCEPT;

/**
 * \brief Destroy and free an iterator
 * @param[in] iter a iterator get by using pilot_get_pi_unit_readings_iter()
 */
DLL_PUBLIC void pilot_pi_unit_readings_iter_destroy(pilot_pi_unit_readings_iter_t* iter) NOEXCEPT;

/**
 * \brief Import one round of benchmark results into a workload session
 * @param[in] wl pointer to the workload struct
 * @param round the round ID to store the results with. You can replace the data of
 * an existing round, or add a new round by setting the round to the value of
 * pilot_get_num_of_rounds()
 * @param work_amount the amount of work of that round (you can set it to equal
 * num_of_unit_readings if you have no special use for this value)
 * @param round_duration the duration of the round
 * @param[in] readings the readings of each PI
 * @param num_of_unit_readings the number of unit readings
 * @param[in] unit_readings the unit readings of each PI, can be NULL if there
 * is no unit readings in this round
 */
DLL_PUBLIC void pilot_import_benchmark_results(pilot_workload_t *wl, size_t round,
                                    size_t work_amount,
                                    nanosecond_type round_duration,
                                    const double *readings,
                                    size_t num_of_unit_readings,
                                    const double * const *unit_readings) NOEXCEPT;

/**
 * \brief Get the amount of work load that will be used for next round
 * @param[in] wl pointer to the workload struct
 * @param[out] needed_work_amount. A returned 0 value doesn't necessarily mean
 * no more work is needed, because the workload may not support setting work
 * amount. Always use the return value to decide if more rounds are needed.
 * If return value is false (no more round is needed) this value can still be
 * useful for further refining the result.
 * @return true if more rounds are needed; false if no more rounds are needed
 */
DLL_PUBLIC bool pilot_next_round_work_amount(const pilot_workload_t *wl, size_t *needed_work_amount) NOEXCEPT;

/**
 * \brief Set the threshold for short round detection
 * \details Any round that is shorter than this threshold will not be used
 * @param[in] wl pointer to the workload struct
 * @param threshold the threshold in seconds
 */
DLL_PUBLIC void pilot_set_short_round_detection_threshold(pilot_workload_t *wl, size_t threshold) NOEXCEPT;

/**
 * \brief Set the required width of confidence interval
 * @param[in] wl pointer to the workload struct
 * @param percent_of_medium set the requirement to a percent of mean, set
 *        this to -1 (or any negative value) to use absolute_value instead
 * @param absolute_value use an absolute value, set this to -1 (or any negative
 *        value to use percent_of_medium instead
 */
DLL_PUBLIC void pilot_set_required_confidence_interval(pilot_workload_t *wl, double percent_of_mean, double absolute_value) NOEXCEPT;

/**
 * \brief Calculate the work amount needed for the round duration to meet
 * short_round_detection_threshold_
 * @param[in] wl pointer to the workload struct
 * @param[out] needed_work_amount. A returned 0 value doesn't necessarily mean
 * no more work is needed, because the workload may not support setting work
 * amount. Always use the return value to decide if more rounds are needed.
 * If return value is false (no more round is needed) this value can still be
 * useful for further refining the result.
 * @return true if more rounds are needed; false if no more rounds are needed
 */
DLL_PUBLIC bool calc_next_round_work_amount_meet_lower_bound(const pilot_workload_t *wl, size_t *needed_work_amount) NOEXCEPT;

/**
 * \brief Calculate the work amount for next round from readings data
 * @param[in] wl pointer to the workload struct
 * @param[out] needed_work_amount. A returned 0 value doesn't necessarily mean
 * no more work is needed, because the workload may not support setting work
 * amount. Always use the return value to decide if more rounds are needed.
 * If return value is false (no more round is needed) this value can still be
 * useful for further refining the result.
 * @return true if more rounds are needed; false if no more rounds are needed
 */
DLL_PUBLIC bool calc_next_round_work_amount_from_readings(const pilot_workload_t *wl, size_t *needed_work_amount) NOEXCEPT;

/**
 * \brief Calculate the work amount for next round from unit readings data
 * @param[in] wl pointer to the workload struct
 * @param[out] needed_work_amount. A returned 0 value doesn't necessarily mean
 * no more work is needed, because the workload may not support setting work
 * amount. Always use the return value to decide if more rounds are needed.
 * If return value is false (no more round is needed) this value can still be
 * useful for further refining the result.
 * @return true if more rounds are needed; false if no more rounds are needed
 */
DLL_PUBLIC bool calc_next_round_work_amount_from_unit_readings(const pilot_workload_t *wl, size_t *needed_work_amount) NOEXCEPT;

/**
 * \brief Calculate the work amount for next round from unit readings data
 * @param[in] wl pointer to the workload struct
 * @param[out] needed_work_amount. A returned 0 value doesn't necessarily mean
 * no more work is needed, because the workload may not support setting work
 * amount. Always use the return value to decide if more rounds are needed.
 * If return value is false (no more round is needed) this value can still be
 * useful for further refining the result.
 * @return true if more rounds are needed; false if no more rounds are needed
 */
DLL_PUBLIC bool calc_next_round_work_amount_from_wps(const pilot_workload_t *wl, size_t *needed_work_amount) NOEXCEPT;


/**
 * \brief Calculate the work amount for next round for getting the desired
 * p-value for comparison
 * @param[in] wl pointer to the workload struct
 * @param[out] needed_work_amount. A returned 0 value doesn't necessarily mean
 * no more work is needed, because the workload may not support setting work
 * amount. Always use the return value to decide if more rounds are needed.
 * If return value is false (no more round is needed) this value can still be
 * useful for further refining the result.
 * @return true if more rounds are needed; false if no more rounds are needed
 */
DLL_PUBLIC bool calc_next_round_work_amount_for_comparison(const pilot_workload_t *wl, size_t *needed_work_amount) NOEXCEPT;

/**
 * \brief Set if WPS analysis should be enabled
 * @param[in] wl pointer to the workload struct
 * @param[in] format_wps_func the function for formatting
 * a work-per-second number for display. Setting this parameter
 * to NULL to disable it.
 * @param enabled if WPS analysis is enabled
 * @param wps_must_satisfy if WPS CI must satisfy
 * @return 0 on success; otherwise error code
 */
DLL_PUBLIC int pilot_set_wps_analysis(pilot_workload_t *wl,
        pilot_pi_display_format_func_t *format_wps_func,
        bool enabled, bool wps_must_satisfy) NOEXCEPT;

/**
 * \brief Set the desired duration for running a session
 * \details Pilot will try to get a result within this desired duration but
 * does not guarantee it. Default to 30 seconds.
  * @param[in] wl pointer to the workload struct
 * @param sec the desired session duration limit
 * @return previous setting
 */
DLL_PUBLIC size_t pilot_set_session_desired_duration(pilot_workload_t *wl, size_t sec) NOEXCEPT;

/**
 * \brief Set the duration limit for running a session. The session will stop after
 * sec seconds.
 * @param[in] wl pointer to the workload struct
 * @param sec the session duration limit; 0 disables limit.
 * @return previous setting
 */
DLL_PUBLIC size_t pilot_set_session_duration_limit(pilot_workload_t *wl, size_t sec) NOEXCEPT;

DLL_PUBLIC double pilot_set_autocorrelation_coefficient(pilot_workload_t *wl, double ac) NOEXCEPT;

/**
 * \brief Set the baseline for comparison
 * This function sets the baseline for a certain reading type of a certain
 * PIID. After setting this baseline, the workload will be kept running until a
 * conclusion can be reached that can reject the null hypothesis, which is that
 * the current workload equals the baseline workload.
 * @param[in] wl pointer to the workload struct
 * @param piid the PIID to set baseline for
 * @param rt the reaing type to set baseline for (readings, unit readings, or WPS)
 * @param baseline_mean
 * @param baseline_sample_size
 * @param baseline_var
 */
DLL_PUBLIC void pilot_set_baseline(pilot_workload_t *wl, size_t piid, pilot_reading_type_t rt,
        double baseline_mean, size_t baseline_sample_size, double baseline_var) NOEXCEPT;

/**
 * \brief Get the comparison baseline
 * This function gets the baseline for a certain reading type of a certain
 * PIID. The baseline is either set by using pilot_set_baseline() or loaded
 * from a summary file by using pilot_load_baseline_file().
 * @param[in] wl pointer to the workload struct
 * @param piid the PIID to get baseline for
 * @param rt the reaing type to get baseline for (readings, unit readings, or WPS)
 * @param p_baseline_mean the pointer for storing the baseline mean
 * @param p_baseline_sample_size the pointer for storing the baseline sample size
 * @param p_baseline_var the pointer for storing the baseline variance
 * @return 0 on success; ERR_NOT_INIT if the baseline is not set
 */
DLL_PUBLIC int pilot_get_baseline(const pilot_workload_t *wl, size_t piid, pilot_reading_type_t rt,
        double *p_baseline_mean, size_t *p_baseline_sample_size,
        double *p_baseline_var) NOEXCEPT;

/**
 * \brief Load baseline data from a CSV file
 * @return 0 on a successful read; ERR_IO on failures
 */
DLL_PUBLIC int pilot_load_baseline_file(pilot_workload_t *wl, const char *filename) NOEXCEPT;

/**
 * \brief Set the lower threshold of sample size used in all statistical
 * analyses. Default to 200.
 * @param[in] wl pointer to the workload struct
 * @param min_sample_size the new minimum sample size
 * @return the old min sample size
 */
DLL_PUBLIC size_t pilot_set_min_sample_size(pilot_workload_t *wl, size_t min_sample_size) NOEXCEPT;

DLL_PUBLIC int _simple_runner(pilot_simple_workload_func_t func,
                   const char *benchmark_name) NOEXCEPT;
DLL_PUBLIC int _simple_runner_with_wa(pilot_simple_workload_with_wa_func_t func,
                           const char *benchmark_name,
                           size_t min_wa DEFAULT_VALUE(0), size_t max_wa DEFAULT_VALUE(ULONG_MAX),
                           size_t short_round_threshold DEFAULT_VALUE(1)) NOEXCEPT;
#define simple_runner(func, ...) \
    _simple_runner(func, #func, ##__VA_ARGS__)
#define simple_runner_with_wa(func, ...) \
    _simple_runner_with_wa(func, #func, ##__VA_ARGS__)

#ifdef __cplusplus
} // namespace pilot
} // extern C
#endif

#endif /* LIBPILOT_HEADER_LIBPILOT_H_ */
