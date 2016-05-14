/*
 * common.h: the common header file shared across pilot-tool
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

#ifndef PILOT_TOOL_INCLUDE_COMMON_H_
#define PILOT_TOOL_INCLUDE_COMMON_H_

#include <boost/log/trivial.hpp>
#include <boost/timer/timer.hpp>
#include <iostream>
#include <string>
#include <vector>

#define debug_log   BOOST_LOG_TRIVIAL(debug)
#define info_log    BOOST_LOG_TRIVIAL(info)
#define warning_log BOOST_LOG_TRIVIAL(warning)
#define error_log   BOOST_LOG_TRIVIAL(error)
#define fatal_log   BOOST_LOG_TRIVIAL(fatal)

// extra indirection so other macros can be used as parameter
// http://stackoverflow.com/questions/6713420/c-convert-integer-to-string-at-compile-time#comment7949445_6713658
#define _stringify(x) #x
#define stringify(x) _stringify(x)

#define SHOULD_NOT_REACH_HERE { fatal_log << __func__ << "():" << stringify(__LINE__) \
    << " Error: shouldn't reach here"; }

namespace pilot {

// all consts go here, they should be named either k_something, or ALL_UPPERCASE
boost::timer::nanosecond_type const ONE_SECOND = 1000000000LL;
size_t const MEGABYTE = 1024*1024;

inline void die_if (bool condition, int error_code = 1, const char *error_msg = NULL) {
    if (!condition) return;

    fatal_log << error_msg;
    exit(error_code);
}

inline void die_if (bool condition, int error_code = 1, const std::string & error_msg = "") {
    if (!condition) return;

    fatal_log << error_msg;
    exit(error_code);
}

/**
 * \brief ASSERT_VALID_POINTER(p) checks that p is a valid pointer and aborts if it is NULL
 * \details This macro is used to check critical input parameters.
 */
#define ASSERT_VALID_POINTER(p) { if (NULL == (p)) { \
        fatal_log << "function " << __func__ << "() called with " #p " as a NULL pointer"; \
        abort(); \
    }}

/**
 * Printing a pair
 * @param o the ostream object
 * @param a the pair for printing
 * @return
 */
template <typename FirstT, typename SecondT>
std::ostream& operator<<(std::ostream &o, const std::pair<FirstT, SecondT> &a) {
    o << "<" << a.first << ", " << a.second << ">";
    return o;
}

/**
 * Printing a vector
 * @param o the ostream object
 * @param a the vector for printing
 * @return
 */
template <typename T>
std::ostream& operator<<(std::ostream &o, const std::vector<T> &a) {
    o << "[";
    if (a.size() > 0) {
        // we cannot use copy() and ostream_iterator here because they insist
        // that operator<<() should be in std.
        for (size_t i = 0; i < a.size() - 1; ++i) {
            o << a[i] << ", ";
        }
        o << a.back();
    }
    o << "]";
    return o;
}

} /* namespace pilot */

#endif /* PILOT_TOOL_INCLUDE_COMMON_H_ */
