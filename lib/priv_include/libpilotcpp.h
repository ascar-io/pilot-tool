/*
 * libpilotcpp.h: the C++ binding (under construction)
 *
 * This C++ binding is still under construction and is not fully tested yet.
 * Use at your own risk. Feedbacks are welcome.
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

#ifndef LIB_PRIV_INCLUDE_LIBPILOTCPP_H_
#define LIB_PRIV_INCLUDE_LIBPILOTCPP_H_

#include <boost/format.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <cmath>
#include <exception>
#include "pilot/libpilot.h"
#include <limits>
#include <map>
#include <memory>
#include <vector>
#include "workload.hpp"

namespace pilot {

class accumulator_base {                      // All accumulators should inherit from
public:
    virtual void operator()(double data) = 0;
    virtual double result() const = 0;
    virtual ~accumulator_base() {};
};

class arithmetic_mean_accumulator : public accumulator_base {
public:
    arithmetic_mean_accumulator() : n(0), sum(0) {}

    virtual void operator ()(double data) {
        sum += data;
        ++n;
    }

    virtual double result() const {
        return sum / static_cast<double>(n);
    }

    ~arithmetic_mean_accumulator() {}
private:
    int    n;
    double sum;
};

class harmonic_mean_accumulator : public accumulator_base {
public:
    harmonic_mean_accumulator() : n(0), har_sum(0) {}

    virtual void operator ()(double data) {
        har_sum += 1.0 / data;
        ++n;
    }

    virtual double result() const {
        return static_cast<double>(n) / har_sum;
    }

    ~harmonic_mean_accumulator() {}
private:
    int    n;
    double har_sum;
};

inline accumulator_base* accumulator_factory(pilot_mean_method_t mean_method) {
    switch (mean_method) {
    case ARITHMETIC_MEAN:
        return new arithmetic_mean_accumulator();
    case HARMONIC_MEAN:
        return new harmonic_mean_accumulator();
    }
    abort();
}

template <typename InputIterator1, typename InputIterator2>
double pilot_cov(InputIterator1 x, InputIterator2 y, size_t n, double x_mean, double y_mean,
        enum pilot_mean_method_t mean_method = ARITHMETIC_MEAN) {
    double sum = 0;
    for (size_t i = 0; i < n; ++i) {
        sum += (double(x[i]) - x_mean) * (double(y[i]) - y_mean);
    }
    return sum / (n - 1);
}

template <typename InputIterator>
double pilot_subsession_auto_cov(InputIterator first, size_t n, size_t q, double sample_mean,
        enum pilot_mean_method_t mean_method = ARITHMETIC_MEAN) {
    // we always use arithmetic mean for the covariance accumulator
    arithmetic_mean_accumulator cov_acc;
    size_t h = n/q;
    if (1 == h) {
        error_log << "cannot calculate covariance for one sample";
        abort();
    }

    double uae, ube;
    std::unique_ptr<accumulator_base> ua_acc(accumulator_factory(mean_method));
    for (size_t a = 0; a < q; ++a)
        (*ua_acc)(*first++);
    uae = ua_acc->result() - sample_mean;

    for (size_t i = 1; i < h; ++i) {
        std::unique_ptr<accumulator_base> ub_acc(accumulator_factory(mean_method));
        for (size_t b = 0; b < q; ++b)
            (*ub_acc)(*first++);
        ube = ub_acc->result() - sample_mean;

        cov_acc(uae * ube);
        uae = ube;
    }
    return cov_acc.result();
}

template <typename InputIterator>
double pilot_subsession_var(InputIterator first, size_t n, size_t q,
        double sample_mean, pilot_mean_method_t mean_method = ARITHMETIC_MEAN) {
    double s = 0;
    size_t h = n/q;  // subsession sample size
    for (size_t i = 0; i < h; ++i) {
        std::unique_ptr<accumulator_base> acc(accumulator_factory(mean_method));
        for (size_t j = 0; j < q; ++j)
            (*acc)(*first++);

        s += pow(acc->result() - sample_mean, 2);
    }
    return s / (h - 1);
}

template <typename InputIterator>
double pilot_subsession_autocorrelation_coefficient(InputIterator first,
        size_t n, size_t q, double sample_mean, pilot_mean_method_t mean_method) {
    if (n / q < 2) {
        return 1;
    }

    double res = pilot_subsession_auto_cov(first, n, q, sample_mean, mean_method) /
                 pilot_subsession_var(first, n, q, sample_mean, mean_method);

    // res can be NaN when the variance is 0, in this case we just return 1,
    // which means the result has high autocorrelation.
    if (std::isnan(res))
        return 1;
    else
        return res;
}

template <typename InputIterator>
double pilot_subsession_mean(InputIterator first, size_t n, pilot_mean_method_t mean_method) {
    std::unique_ptr<accumulator_base> acc(accumulator_factory(mean_method));
    while (n-- != 0)
        (*acc)(*first++);
    return acc->result();
}

template <typename InputIterator>
int pilot_optimal_subsession_size(InputIterator first, const size_t n,
                                  pilot_mean_method_t mean_method,
                                  double max_autocorrelation_coefficient = 0.1) {
    if (n <= 1) {
        debug_log << "cannot calculate covariance for " << n << " sample(s)";
        return -1;
    }
    double sm = pilot_subsession_mean(first, n, mean_method);
    double cov;
    for (size_t q = 1; q != n / 3 + 1; ++q) {
        cov = pilot_subsession_autocorrelation_coefficient(first, n, q, sm, mean_method);
        trace_log << __func__ << "(): subsession size: " << q << ", auto. cor. coef.: " << cov;
        if (std::abs(cov) <= max_autocorrelation_coefficient)
            return q;
    }
    return -1;
}

/**
 * \brief Calculate two sided confidence interval on the mean assuming the population variance is unknown
 *
 * If the population variance is known, we can use z* as the critical value. Here we don't know the
 * population variance so we have to use Student's t distribution with n-1 degrees of freedom. When ci_type
 * is BINOMIAL_PROPORTION, subsession size (q) is usually set to 1.
 *
 * @tparam InputIterator type of the input iterator
 * @param first iterator to the beginning of the data
 * @param n length of data
 * @param q subsession size
 * @param confidence_level desired confidence level
 * @param mean_method method to calculate mean: arithmetic or harmonic
 * @param ci_type confidence interval type: sample mean or binomial proportion
 * @return width of the confidence interval
 */
template <typename InputIterator>
double pilot_subsession_confidence_interval(InputIterator first, size_t n,
        size_t q, double confidence_level, pilot_mean_method_t mean_method, pilot_confidence_interval_type_t ci_type) {
    // See http://www.boost.org/doc/libs/1_66_0/libs/math/doc/html/math_toolkit/stat_tut/weg/st_eg/tut_mean_intervals.html
    // for explanation of the code. The same formula can also be found at
    // [Ferrari78], page 59 and [Le Boudec15], page 34.
    using namespace boost::math;

    size_t h = n / q;
    students_t dist(h - 1);
    // T is called z' in [Ferrari78], page 60 and is z in https://en.m.wikipedia.org/wiki/Binomial_proportion_confidence_interval.
    double T = ::boost::math::quantile(complement(dist, (1 - confidence_level) / 2));

    double sm = pilot_subsession_mean(first, n, mean_method);
    double var;
    switch (ci_type) {
        case SAMPLE_MEAN:
            var = pilot_subsession_var(first, n, q, sm, mean_method);
            return T * sqrt(var / double(h)) * 2;
        case BINOMIAL_PROPORTION:
            if (sm > 1 || sm < 0) {
                throw std::runtime_error("The data mean for BINOMIAL_PROPORTION confidence interval calculation is not within [0, 1]");
            }
            return T * sqrt(sm * (1-sm) / h) * 2;
        default:
            throw std::runtime_error("Invalid value for ci_type, must be SAMPLE_MEAN or BINOMIAL_PROPORTION");
    }
}

/**
 * See pilot_optimal_sample_size_p()
 */
template <typename InputIterator> bool
pilot_optimal_sample_size(InputIterator first, size_t n,
                          double confidence_interval_width,
                          pilot_mean_method_t mean_method,
                          size_t *q, size_t *opt_sample_size,
                          pilot_confidence_interval_type_t ci_type = SAMPLE_MEAN,
                          double confidence_level = 0.95,
                          double max_autocorrelation_coefficient = 0.1) {
    using namespace boost::math;
    assert(q);
    assert(opt_sample_size);

    int res;
    switch (ci_type) {
        case SAMPLE_MEAN:
            res = pilot_optimal_subsession_size(first, n, mean_method, max_autocorrelation_coefficient);
            if (res < 0) return false;
            *q = static_cast<size_t>(res);
            trace_log << "optimal subsession size (q) = " << *q;
            break;
        case BINOMIAL_PROPORTION:
            *q = 1;
            trace_log << "optimal subsession size (q) for binomial proportion CI is always " << *q;
            break;
        default:
            throw std::runtime_error("Invalid value for ci_type, must be SAMPLE_MEAN or BINOMIAL_PROPORTION");
    }

    size_t h = n / *q;
    students_t dist(h-1);
    // T is called z' in [Ferrari78], page 60.
    double T = ::boost::math::quantile(complement(dist, (1 - confidence_level) / 2));
    trace_log << "T score for " << 100 * confidence_level << "% confidence level = " << T;
    trace_log << "expected CI width: " << confidence_interval_width;
    double e = confidence_interval_width / 2;

    double sm = pilot_subsession_mean(first, n, mean_method);
    double var = pilot_subsession_var(first, n, *q, sm, mean_method);
    *opt_sample_size = ceil(var * pow(T / e, 2));
    trace_log << str(boost::format("number of samples required: %1% (desired sample size %2% x opt. subsession size %3%)")
                 % ((*opt_sample_size) * (*q)) % *opt_sample_size % *q);
    return true;
}

template <typename T1, typename T2>
void simple_regression_model(const std::vector<T1> &x, const std::vector<T2> &y, double *alpha, double *v) {
    double x_mean = pilot_subsession_mean(x.data(), x.size(), ARITHMETIC_MEAN);
    double y_mean = pilot_subsession_mean(y.data(), y.size(), ARITHMETIC_MEAN);
    double x_var = pilot_subsession_var(x.data(), x.size(), 1, x_mean, ARITHMETIC_MEAN);
    double xy_cov = pilot_cov(x.data(), y.data(), x.size(), x_mean, y_mean, ARITHMETIC_MEAN);
    *v = xy_cov / x_var;
    *alpha = y_mean - (*v) * x_mean;
}

/**
 * Perform warm-up phase detection and removal on readings using the linear
 * regression method
 * @param rounds
 * @param round_work_amounts
 * @param round_durations
 * @param autocorrelation_coefficient_limit
 * @param duration_threshold any round whose duration is less than this threshold is discarded
 * @param v
 * @param ci_width
 * @return 0 on success; ERR_NOT_ENOUGH_DATA when there is not enough sample
 * for calculate v; ERR_NOT_ENOUGH_DATA_FOR_CI when there is enough data for
 * calculating v but not enough for calculating confidence interval.
 */
template <typename WorkAmountInputIterator, typename RoundDurationInputIterator>
int pilot_wps_warmup_removal_lr_method(size_t rounds, WorkAmountInputIterator round_work_amounts_raw,
        RoundDurationInputIterator round_durations_raw,
        float autocorrelation_coefficient_limit, nanosecond_type duration_threshold,
        double *wps_alpha, double *wps_v,
        double *wps_v_ci, double *ssr_out = NULL, double *ssr_percent_out = NULL,
        size_t *subsession_sample_size = NULL, size_t *out_q = NULL) {
    // first we create copies of round_work_amounts and round_durations with
    // rounds that are shorter than round_durations filtered out
    std::vector<size_t> round_work_amounts;
    std::vector<nanosecond_type> round_durations;
    // TODO: we can probably switch to using a more efficient iterator to skip
    // those rounds that are too short. See FB#2947.
    for (size_t i = 0; i < rounds; ++i) {
        if (round_durations_raw[i] > duration_threshold) {
            round_work_amounts.push_back(round_work_amounts_raw[i]);
            round_durations.push_back(round_durations_raw[i]);
        }
    }

    if (round_work_amounts.size() < 3) {
        debug_log << __func__ << "() doesn't have enough samples after filtering using duration threshold";
        return ERR_NOT_ENOUGH_DATA;
    }

    // then check for auto-correlation
    std::vector<double> naive_v_per_round;
    for (size_t i = 0; i < round_work_amounts.size(); ++i) {
        naive_v_per_round.push_back(static_cast<double>(round_work_amounts[i]) / static_cast<double>(round_durations[i]));
    }
    int q = pilot_optimal_subsession_size(naive_v_per_round.begin(), naive_v_per_round.size(), HARMONIC_MEAN, autocorrelation_coefficient_limit);
    if (q < 0) {
        debug_log << __func__ << "() samples' autocorrelation coefficient too high; need more samples";
        return ERR_NOT_ENOUGH_DATA;
    }
    if (out_q) { *out_q = q; }
    debug_log << "WPS analysis: optimal subsession size (q) = " << q;
    size_t h = round_work_amounts.size() / q;
    if (subsession_sample_size) *subsession_sample_size = h;
    if (h < 3) {
        debug_log << __func__ << "() doesn't have enough samples (<3) after subsession grouping";
        return ERR_NOT_ENOUGH_DATA;
    }
    std::vector<size_t> subsession_work_amounts;
    std::vector<nanosecond_type> subsession_round_durations;

    size_t subsession_sum_wa = 0;
    nanosecond_type subsession_sum_dur = 0;
    size_t total_sum_wa = 0;
    nanosecond_type total_sum_dur = 0;
    // convert input into subsession data by grouping every q samples
    for (size_t i = 0; i < rounds; ++i) {
        subsession_sum_wa  += round_work_amounts[i];
        subsession_sum_dur += round_durations[i];
        total_sum_wa += round_work_amounts[i];
        total_sum_dur += round_durations[i];
        if (i % size_t(q) == size_t(q) - 1) {
            subsession_work_amounts.push_back(subsession_sum_wa);
            subsession_round_durations.push_back(subsession_sum_dur);
            subsession_sum_wa = 0;
            subsession_sum_dur = 0;
        }
    }

    double wps_inv_v;
    {
        double wpns_inv_v;     // invert v of work amount per nanosecond
        simple_regression_model(subsession_work_amounts, subsession_round_durations, wps_alpha, &wpns_inv_v);
        wps_inv_v = wpns_inv_v / ONE_SECOND;
        *wps_alpha /= ONE_SECOND;
        *wps_v = ONE_SECOND / wpns_inv_v;
    }

    double sub_session_ssr = 0;
    for (size_t i = 0; i < subsession_work_amounts.size(); ++i) {
        double wa = double(subsession_work_amounts[i]);
        double dur = double(subsession_round_durations[i]) / ONE_SECOND;
        sub_session_ssr += pow(*wps_alpha + wps_inv_v * wa - dur, 2);
    }
    debug_log << __func__ << "(): sub_session_ssr: " << sub_session_ssr;
    double ssr = 0;
    double dur_sum = 0;
    for (size_t i = 0; i < rounds; ++i) {
        double wa = double(round_work_amounts_raw[i]);
        double dur = double(round_durations_raw[i]) / ONE_SECOND;
        ssr += pow(*wps_alpha + wps_inv_v * wa - dur, 2);
        dur_sum += dur;
    }
    debug_log << __func__ << "(): ssr: " << ssr;
    if (ssr_out) *ssr_out = ssr;
    if (ssr_percent_out) *ssr_percent_out = sqrt(ssr) / dur_sum;

    double sigma_sqr = sub_session_ssr / (h - 2);
    double wa_mean = pilot_subsession_mean(round_work_amounts.begin(), round_work_amounts.size(), ARITHMETIC_MEAN);
    double sum_var = pilot_subsession_var(round_work_amounts.begin(), round_work_amounts.size(), q, wa_mean, ARITHMETIC_MEAN) * (rounds -1);
    double std_err_v = sqrt(sigma_sqr / sum_var);
    double inv_v_ci = 2 * std_err_v;
    // inv_v - inv_v_ci might be negative so we have to use abs() here
    *wps_v_ci = std::abs( 1.0 / (wps_inv_v - inv_v_ci) - 1.0 / (wps_inv_v + inv_v_ci) );
    debug_log << __func__ << "(): result wps_alpha " << *wps_alpha << ", wps_v " << *wps_v << ", wps_v_ci " << *wps_v_ci;
    return 0;
}

} // namespace pilot

#endif /* LIB_PRIV_INCLUDE_LIBPILOTCPP_H_ */
