/*
 * pilot_tui.hpp: C++ classes for Text User Interface
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

#ifndef LIB_INTERFACE_INCLUDE_PILOT_TUI_HPP_
#define LIB_INTERFACE_INCLUDE_PILOT_TUI_HPP_

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/frontend_requirements.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/shared_ptr.hpp>
// for fomatting timestamp in log
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/support/date_time.hpp>

#include <cdk.h>
#include <curses.h>
#include <exception>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <term.h>
#include <vector>
#include "workload.hpp"

namespace pilot {

#define QUIT            CTRL('Q')
#define ESCAPE          CTRL('[')

const chtype DEFAULT_COLOR = COLOR_PAIR(5);

class tui_exception : std::exception {
private:
    std::string err_msg_;
public:
    tui_exception(std::string msg) : err_msg_(msg) {}
    const char* what() const noexcept {
        return err_msg_.c_str();
    }
};

/**
 * \brief This is a model class for tasks information as in the MVC architecture
 */
class TaskList {
private:
    std::vector<char*> tasks_;
    std::vector<bool> enabled_;
public:
    TaskList() {
        tasks_.push_back(strdup("Warm-up removal"));
        tasks_.push_back(strdup("Overhead detection"));
        tasks_.push_back(strdup("Confid. interv. calc"));
        tasks_.push_back(strdup("Percentile calc"));
        tasks_.push_back(strdup("Refining results"));
        tasks_.push_back(strdup("Save results"));
        enabled_.push_back(true);
        enabled_.push_back(false);
        enabled_.push_back(true);
        enabled_.push_back(false);
        enabled_.push_back(false);
        enabled_.push_back(true);
    }
    ~TaskList() {
        for (auto s : tasks_)
            free(s);
    }
    char** data(void) {
        return tasks_.data();
    }
    const std::vector<bool>& enabled(void) { return enabled_; }
    size_t size(void) {
        return tasks_.size();
    }
};

class BoxedWidget {
private:
    int     w_;      //! the width of the box
    int     h_;      //! the height of the box
    WINDOW *wind_;

    void draw(void) {
        wind_ = newwin (h_, w_, y_, x_);
        cdk_screen_ = initCDKScreen(wind_);
        wbkgd(wind_, DEFAULT_COLOR | A_BOLD);
        box (wind_, ACS_VLINE, ACS_HLINE);
        writeChar(cdk_screen_->window,
                  (w_ - title_.size()) / 2,
                  0,
                  const_cast<char*>(title_.c_str()),
                  HORIZONTAL, 0,
                  title_.size());
    }
protected:
    CDKSCREEN *cdk_screen_;
    int x_;      //! the X of the box
    int y_;      //! the Y of the box
    int inner_w_; //! the width of the space for the inner widget
    int inner_h_; //! the height of the space for the inner widget
public:
    const std::string title_;

    virtual void refresh(void) {
        refreshCDKScreen(cdk_screen_);
    }

    BoxedWidget(int h, int w, int y, int x, const std::string &title)
    : w_(w), h_(h), wind_(NULL), x_(x), y_(y), title_(title) {
        inner_w_ = w_ - 2;
        inner_h_ = h_ - 2;
        draw();
    }
    virtual ~BoxedWidget() {
        if (cdk_screen_) destroyCDKScreen(cdk_screen_);
        if (wind_) delwin(wind_);
    }
};

class TaskBox {
private:
    int x_;      //! the X of the box
    int y_;      //! the Y of the box
    int w_;      //! the width of the box
    int h_;      //! the height of the box
    CDKSCREEN *cdk_screen_;
    CDKSELECTION *selection_list_;
    CDKLABEL  *title_label_;
    std::vector<char*> items_;
    TaskList *task_list_;
    const char *title_[1] = {" TASK LIST "};

    const char *choice_list_[2] = {"   ", " X "};

    void draw(void) {
        selection_list_ = newCDKSelection(cdk_screen_,
                                    x_,
                                    y_,
                                    NONE,
                                    h_,
                                    w_,
                                    NULL,
                                    task_list_->data(),
                                    task_list_->size(),
                                    const_cast<char**>(choice_list_),
                                    2,
                                    A_REVERSE,
                                    TRUE,
                                    FALSE);
        for (size_t i = 0; i < task_list_->enabled().size(); ++i) {
            selection_list_->selections[i] = task_list_->enabled()[i] ? 1 : 0;
        }
        setCDKScrollBackgroundAttrib(selection_list_, DEFAULT_COLOR | A_BOLD);
        title_label_ = newCDKLabel(cdk_screen_, x_ + (w_ - strlen(title_[0])) / 2, y_,
                                   const_cast<char**>(title_), 1, FALSE, FALSE);
        setCDKLabelBackgroundAttrib(title_label_, DEFAULT_COLOR | A_BOLD);
    }
public:
    TaskBox(CDKSCREEN *cdk_screen, int h, int w, int y, int x, TaskList *task_list)
    : x_(x), y_(y), w_(w), h_(h), cdk_screen_(cdk_screen),
      task_list_(task_list) {
        draw();
    }

    ~TaskBox() {
        if (selection_list_) destroyCDKScroll(selection_list_);
        if (title_label_) destroyCDKLabel(title_label_);
    }
};

class SummaryBox : public BoxedWidget {
private:
    const std::vector<pilot_pi_info_t> * const pi_info_vec_p_;
    const pilot_display_format_functor &format_wps_;
    static const int kLinesPerPI = 2;

    pilot_analytical_result_t wi_;
    std::stringstream draw_buf_;
    int next_draw_pos_x_;
    int next_draw_pos_y_; //! used by draw_data_line to track the location of the next line
    int prev_max_lines_;  //! the maximum number of lines reached so far; next draw needs to do screen clear up to this mark

    void use_default_color(void) {
        wbkgd(cdk_screen_->window, DEFAULT_COLOR | A_BOLD);
    }

    void use_highlight_color(void) {
        wbkgd(cdk_screen_->window, COLOR_PAIR(29) | A_BOLD);
    }
    /**
     * Print a string at next_draw_pos without considering line folding
     * @param s
     */
    void print_str(const std::string &s) {
        writeChar(cdk_screen_->window,
                  1 + next_draw_pos_x_,
                  next_draw_pos_y_,
                  const_cast<char*>(s.c_str()),
                  HORIZONTAL, 0,
                  s.size());
        next_draw_pos_x_ += s.size();
    }
    /**
     * Flush draw_buf_, taking care of line folding
     */
    void flush_buf(void) {
        while (draw_buf_.str().size() > 0) {
            int avail_space = inner_w_ - next_draw_pos_x_;
            if (avail_space < int(draw_buf_.str().size())) {
                print_str(draw_buf_.str().substr(0, avail_space));
                next_draw_pos_x_ = 0;
                ++next_draw_pos_y_;
                if (int(draw_buf_.str().size()) == avail_space) {
                    draw_buf_.str(std::string()); draw_buf_.clear();
                    return;
                } else {
                    draw_buf_.str(draw_buf_.str().substr(avail_space, draw_buf_.str().size() - avail_space));
                }
            } else {
                print_str(draw_buf_.str());
                draw_buf_.str(std::string()); draw_buf_.clear();
                return;
            }
        }
    }
    void flush_buf_new_line(void) {
        flush_buf();
        for (int i = next_draw_pos_x_; i < inner_w_; ++i) {
            draw_buf_ << " ";
        }
        flush_buf();
        next_draw_pos_x_ = 0;
        ++next_draw_pos_y_;
    }

    template <typename T>
    void draw_data_line(std::string label, T data, std::string tail) {
        // draw label
        use_default_color();
        draw_buf_ << label;
        flush_buf();
        // draw data
        int data_width = inner_w_ - label.size();
        if (tail.size() != 0) data_width -= tail.size();
        use_highlight_color();
        draw_buf_ << std::setw(data_width) << std::setprecision(3) << data;
        // does the data fill in one line
        if (int(draw_buf_.str().size()) > inner_w_ - next_draw_pos_x_) {
            // print data and tail on next line
            draw_buf_.str(std::string()); draw_buf_.clear();
            flush_buf_new_line();
            draw_buf_ << std::setw(inner_w_ - tail.size()) << data;
        }
        flush_buf();
        //draw tail
        use_default_color();
        draw_buf_ << tail;
        flush_buf_new_line();
    }
    /**
     * Draw string s with decor "==" around it
     * @param s the string to draw
     */
    void draw_with_decor(std::string s) {
        // decor is the ====== in the PI separator
        int decor_len = (inner_w_ - s.size() - 2/*spaces*/) / 2;
        for (int i = 0; i < decor_len; ++i) draw_buf_ << '=';
        draw_buf_ << " " << s << " ";
        for (int i = decor_len + s.size() + 2; i < inner_w_; ++i) {
            draw_buf_ << '=';
        }
        use_default_color();
        flush_buf_new_line();
    }
    void draw(void) {
        next_draw_pos_y_ = 1;
        if (wi_.num_of_pi != 0) {
            draw_data_line("total rounds: ", wi_.num_of_rounds, "");

            for (size_t piid = 0; piid < wi_.num_of_pi; ++piid) {
                const std::string &pi_name = (*pi_info_vec_p_)[piid].name;
                const std::string &pi_unit = std::string(" ") + (*pi_info_vec_p_)[piid].unit;
                const pilot_display_format_functor &format_r = (*pi_info_vec_p_)[piid].format_reading;
                const pilot_display_format_functor &format_ur = (*pi_info_vec_p_)[piid].format_unit_reading;

                // draw the PI name as the separator
                draw_with_decor(pi_name);

                if (0 != wi_.readings_num[piid]) {
                    draw_buf_ << "READINGS ANALYSIS";
                    flush_buf_new_line();
                    draw_data_line("mean: ", format_r(NULL, wi_.readings_mean[piid]), pi_unit);
                } else {
                    draw_buf_ << "NO READING DATA";
                    flush_buf_new_line();
                }

                draw_buf_ << "UNIT-READINGS ANALYSIS";
                flush_buf_new_line();
                size_t cur_ur = wi_.unit_readings_num[piid];
                draw_data_line("sample size: ", cur_ur, "");
                if (0 == wi_.unit_readings_num[piid]) continue;
                double sm = wi_.unit_readings_mean[piid];
                double var = wi_.unit_readings_var[piid];
                double var_rt = var / sm;
                draw_data_line("sample mean: ", format_ur(NULL, sm), pi_unit);
                draw_data_line("variance: ", var_rt * format_ur(NULL, sm), pi_unit);

                use_default_color();
                std::string label = "variance to mean ratio: ";
                draw_buf_ << label;
                flush_buf();
                use_highlight_color();
                draw_buf_ << std::setprecision(3)
                          << std::setw(inner_w_ - label.size() - 1)
                          << var_rt * 100 << "%";
                flush_buf_new_line();

                draw_data_line("autocor. coef.: ",
                               wi_.unit_readings_autocorrelation_coefficient[piid],
                               "");
                size_t q = wi_.unit_readings_optimal_subsession_size[piid];
                draw_data_line("optimal subsession size (q): ", q, "");
                double subvar = wi_.unit_readings_optimal_subsession_var[piid];
                double subvar_rt = subvar / sm;
                draw_data_line("subsession var: ", subvar_rt * format_ur(NULL, sm), "");

                use_default_color();
                label = "subvar. to mean ratio: ";
                draw_buf_ << label;
                flush_buf();
                use_highlight_color();
                draw_buf_ << std::setw(inner_w_ - label.size() - 1) << subvar_rt * 100 / sm << "%";
                flush_buf_new_line();

                size_t min_ur = wi_.unit_readings_required_sample_size[piid];
                draw_data_line("min. sample size:", min_ur, "");
                draw_data_line("current sample size: ", cur_ur, "");

                use_highlight_color();
                if (cur_ur >= min_ur && cur_ur / q >= 200) {
                    draw_buf_ << "Sample size large enough";
                } else {
                    if (cur_ur < min_ur) {
                        draw_buf_ << "Sample size too small";
                    } else {
                        draw_buf_ << "Sample size MIGHT be too small" << std::endl;
                    }
                }
                flush_buf_new_line();

                double ci = wi_.unit_readings_optimal_subsession_ci_width[piid];
                double ci_rt = ci / sm;
                double ci_low = format_ur(NULL, sm) * (1 - ci_rt/2);
                double ci_high = format_ur(NULL, sm) * (1 + ci_rt/2) ;
                use_default_color();
                draw_buf_ << "95% confidence interval (CI): ";
                flush_buf_new_line();

                // calculate the len of the CI line
                std::stringstream buf;
                buf << std::setprecision(4);
                buf << "[" << ci_low << ", " << ci_high << "]" << pi_unit;
                int ci_width = buf.str().size();

                draw_buf_ << std::setprecision(4);
                draw_buf_ << std::setw(inner_w_ - ci_width + 1) << std::right << "[";
                flush_buf();
                use_highlight_color();
                draw_buf_ << ci_low;
                flush_buf();
                use_default_color();
                draw_buf_ << ", ";
                flush_buf();
                use_highlight_color();
                draw_buf_ << ci_high;
                flush_buf();
                use_default_color();
                draw_buf_ << "]" << pi_unit;
                flush_buf_new_line();

                draw_data_line("CI width: ", ci_high - ci_low, pi_unit);

                use_highlight_color();
                std::string tail = " of mean";
                draw_buf_ << std::setw(inner_w_ - tail.size() - 1) << ci_rt * 100 << "%";
                flush_buf();
                use_default_color();
                draw_buf_ << tail;
                flush_buf_new_line();
            } /* loop for all PI */
            draw_with_decor("WORK-PER-SECOND ANALYSIS");
            draw_data_line("naive mean: ", wi_.wps_harmonic_mean, "");
            if (wi_.wps_has_data) {
                draw_data_line("WPS alpha: ", format_wps_(NULL, wi_.wps_alpha), "");
                draw_data_line("WPS v: ", format_wps_(NULL, wi_.wps_v), "");
                draw_data_line("WPS v CI: ", format_wps_(NULL, wi_.wps_v_ci), "");
            } else {
                draw_buf_ << "Not enough data for WPS analysis";
                flush_buf_new_line();
            }

            // if this refresh cycle has fewer lines than previous cycle,
            // we need to clear the rest of the lines from previous cycle
            int this_max_lines = next_draw_pos_y_;
            while (next_draw_pos_y_ < prev_max_lines_) {
                draw_buf_ << std::setw(inner_w_) << " ";
                flush_buf_new_line();
            }
            prev_max_lines_ = this_max_lines;

            refresh();
        }
    }
public:

    void show_workload_info(const pilot_analytical_result_t &wi) {
        wi_ = wi;
        draw();
    }
    SummaryBox(int h, int w, int y, int x,
               const std::vector<pilot_pi_info_t> *pi_info_vec_p,
               const pilot_display_format_functor &format_wps)
        : BoxedWidget(h, w, y, x, " WORKLOAD SUMMARY "),
          pi_info_vec_p_(pi_info_vec_p), format_wps_(format_wps),
          next_draw_pos_x_(0), next_draw_pos_y_(0),
          prev_max_lines_(0) {
        draw();
    }
};

class MsgBox {
private:
    int x_;      //! the X of the box
    int y_;      //! the Y of the box
    int w_;      //! the width of the box
    int h_;      //! the height of the box
    CDKSCREEN  *cdk_screen_;
    CDKSWINDOW *swindow_;
    CDKLABEL   *title_label_;
    const char *title_[1] = {" MESSAGES "};

    void draw(void) {
        swindow_ = newCDKSwindow(cdk_screen_,
                                 x_,
                                 y_,
                                 h_,
                                 w_,
                                 NULL,
                                 500,
                                 TRUE,
                                 FALSE);
        setCDKSwindowBackgroundAttrib(swindow_, DEFAULT_COLOR | A_BOLD);
        title_label_ = newCDKLabel(cdk_screen_, x_ + (w_ - strlen(title_[0])) / 2, y_,
                                   const_cast<char**>(title_), 1, FALSE, FALSE);
        setCDKLabelBackgroundAttrib(title_label_, DEFAULT_COLOR | A_BOLD);
    }
public:
    MsgBox(CDKSCREEN *cdk_screen, int h, int w, int y, int x)
    : x_(x), y_(y), w_(w), h_(h - 1), cdk_screen_(cdk_screen) {
        draw();
    }
    ~MsgBox() {
        if (swindow_) destroyCDKSwindow(swindow_);
    }
    void add_msg(const std::string &msg) {
        assert (swindow_);
        addCDKSwindow (swindow_, msg.c_str(), BOTTOM);
    }
};

/**
 * \brief This is the view class as in the MVC architecture
 * \details This class is thread-safe
 */
class PilotTUI {
protected:
    std::mutex      lock_;

    TaskList   task_list_;
    const std::vector<pilot_pi_info_t> *pi_info_vec_p_;
    const pilot_display_format_functor &format_wps_;

    WINDOW     *curses_win_;
    CDKSCREEN  *cdk_screen_;

    CDKLABEL   *title_bar_;
    CDKLABEL   *progress_bar_;
    CDKLABEL   *bottom_bar_a_;
    CDKLABEL   *bottom_bar_b_;
    TaskBox    *task_box_;
    SummaryBox *summary_box_;
    MsgBox     *msg_box_;
    int         left_col_width_;
    const int  task_box_height_;
    const char *_title[1] = {"</57> PILOT v0.2 <#VL> "};
    const char *bottom_txt_[1] = {"</57> <!57></9>F1<!9></57> Help  <!57></9>Q<!9></57> Quit<!57>"};
    int         bottom_txt_len_ = 16;

    std::stringstream msg_ss_buf_; // used by the << operator

    // this function must not be called in parallel
    void _flush_msg_ss_buf_nonlock(void) {
        if (msg_ss_buf_.str().size() == 0)
            return;
        // here we use a prefix to add some color if the output has no color yet
        if (msg_ss_buf_.str()[0] != '<' ) {
            static const char prefix[] = "</21>";
            char *buf = new char[sizeof(prefix) + msg_ss_buf_.str().size()];
            strcpy(buf, prefix);
            strcpy(buf + sizeof(prefix) - 1, msg_ss_buf_.str().c_str());
            msg_box_->add_msg(buf);
            delete[] buf;
        } else {
            msg_box_->add_msg(msg_ss_buf_.str().c_str());
        }
        // clear msg_ss_buf_
        msg_ss_buf_.str(std::string());
        msg_ss_buf_.clear();
    }

    class PilotTUILogBackend :
        public boost::log::sinks::basic_formatted_sink_backend<
            char,
            boost::log::sinks::concurrent_feeding
        > {
    protected:
        PilotTUI *pilot_tui_;
    public:
        explicit PilotTUILogBackend(PilotTUI *p) : pilot_tui_(p) {}

        void consume(boost::log::record_view const& rec, string_type const& msg) {
            //*pilot_tui_ << rec.attribute_values()["Message"].extract<std::string>() << std::endl;
            *pilot_tui_ << msg << std::endl;
        }
    };

    // set the boost log to use me as the sink
    void set_logging_sink(void) {
        namespace logging = boost::log;
        namespace src = boost::log::sources;
        namespace sinks = boost::log::sinks;
        namespace keywords = boost::log::keywords;
        namespace expr = boost::log::expressions;

        logging::add_common_attributes();
        boost::shared_ptr< logging::core > core = logging::core::get();

        // Create a backend and attach a stream to it
        boost::shared_ptr<PilotTUILogBackend> backend = boost::make_shared<PilotTUILogBackend>(this);

        // Wrap it into the frontend and register in the core.
        // The backend requires synchronization in the frontend.
        typedef sinks::synchronous_sink<PilotTUILogBackend> sink_t;
        boost::shared_ptr<sink_t> sink(new sink_t(backend));
        sink->set_formatter
        (
            expr::format("%1%:[%2%] <%3%> %4%")
                % expr::attr< unsigned int >("LineID")
                % expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
                % logging::trivial::severity
                % expr::smessage
        );
        core->add_sink(sink);
        pilot_remove_console_log_sink();
    }

public:
    PilotTUI(const std::vector<pilot_pi_info_t> *pi_info_vec_p,
             const pilot_display_format_functor &format_wps) :
        pi_info_vec_p_(pi_info_vec_p), format_wps_(format_wps),
        title_bar_(NULL), progress_bar_(NULL), task_box_(NULL),
        summary_box_(NULL), msg_box_(NULL),
        task_box_height_(task_list_.size() + 2) {
        // the width of the left column is [35, 50]
        left_col_width_ = std::max(COLS / 3, 35);
        left_col_width_ = std::min(50, left_col_width_);

        int errret;
        if (ERR == setupterm(NULL, fileno(stdout), &errret)) {
            throw tui_exception("terminal does not support ncurses");
        }

        curses_win_ = initscr();
        // Hide the cursor
        curs_set(0);
        cdk_screen_ = initCDKScreen(curses_win_);
        initCDKColor();
        draw();
        set_logging_sink();
        refresh();
    }
    ~PilotTUI() {
        delete msg_box_;
        delete summary_box_;
        delete task_box_;
        destroyCDKLabel(bottom_bar_b_);
        destroyCDKLabel(bottom_bar_a_);
        destroyCDKLabel(title_bar_);
        destroyCDKScreen(cdk_screen_);
        endCDK();
    }
    void draw() {
        std::lock_guard<std::mutex> lock(lock_);

        if (title_bar_) {
            // this is a redraw
            destroyCDKLabel(title_bar_);
            destroyCDKLabel(bottom_bar_b_);
            destroyCDKLabel(bottom_bar_a_);
            delete msg_box_;
            delete summary_box_;
            delete task_box_;
        }
        title_bar_ = newCDKLabel(cdk_screen_, 0, 0,
                                 const_cast<char**>(_title), 1, FALSE, FALSE);
        // draw the first half the bottom bar
        bottom_bar_a_ = newCDKLabel(cdk_screen_, 0, LINES - 1,
                                    const_cast<char**>(bottom_txt_), 1, FALSE, FALSE);
        // draw the second half the bottom bar, which is empty that fills the rest of the line
        char *buf[1];
        int second_half_len = COLS - bottom_txt_len_ + 1;
        buf[0] = new char[second_half_len];
        memset(buf[0], ' ', second_half_len - 1);
        buf[0][second_half_len - 1] = 0;
        bottom_bar_b_ = newCDKLabel(cdk_screen_, bottom_txt_len_, LINES - 1, buf, 1, FALSE, FALSE);
        setCDKLabelBackgroundAttrib(bottom_bar_b_, COLOR_PAIR(57));
        delete[] buf[0];

        task_box_ = new TaskBox(cdk_screen_,
                                task_box_height_,
                                left_col_width_,
                                1,
                                0,
                                &task_list_);
        summary_box_ = new SummaryBox(LINES - task_box_height_ - 2,
                                      left_col_width_,
                                      task_box_height_ + 1,
                                      0, pi_info_vec_p_, format_wps_);
        msg_box_ = new MsgBox(cdk_screen_, LINES - 2, COLS - left_col_width_, 1, left_col_width_);
    }
    void refresh() {
        std::lock_guard<std::mutex> lock(lock_);
        refreshCDKScreen(cdk_screen_);
        summary_box_->refresh();
    }

    void event_loop() {
        fd_set fds;
        int ch = ERR;
        do {
            switch (ch) {
            default:
                beep();
                break;
            case ERR:
                break;
            }
            while (true) {
                /*  select() modifies the fd_sets passed to it,
                 *  so zero and set them prior to each call.     */
                FD_ZERO(&fds);
                FD_SET(STDIN_FILENO, &fds);

                int status = select(STDIN_FILENO + 1, &fds, NULL, NULL, NULL);
                if (status == -1) {
                    if ( errno != EINTR) {
                        perror("error calling select()");
                        exit(-status);
                    }
                    continue;
                }
                else if (FD_ISSET(STDIN_FILENO, &fds)) {
                    /*  Only call getch() if input is ready.
                     *  getch() will not block when we do it this way.  */
                    std::lock_guard<std::mutex> lock(lock_);
                    if ((ch = getch()) == ERR) {
                        std::cerr << "ERR returned from getch()" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    break;
                } else {
                    std::cerr << "Internal error with select()" << std::endl;
                    exit(EXIT_FAILURE);
                }
            } /* unbusy loop until a key is pressed */
        } while (ch != QUIT && ch != ESCAPE && ch != 'Q' && ch != 'q');
        printf ("%d\n", ch);
    }
    void flush(void) {
        std::lock_guard<std::mutex> lock(lock_);
        _flush_msg_ss_buf_nonlock();
    }
    /**
     * \brief Flush the message std::stringstream buffer
     */
    template <typename R>
    PilotTUI& operator<<(const R &t) {
        std::lock_guard<std::mutex> lock(lock_);
        msg_ss_buf_ << t;
        return *this;
    }
    PilotTUI& operator<<(const std::stringstream &s) {
        *this << s.str().c_str();
        return *this;
    }
    PilotTUI& operator<<(char *t) {
        return (*this) << const_cast<const char*>(t);
    }
    PilotTUI& operator<<(const char *t) {
        std::lock_guard<std::mutex> lock(lock_);
        while (*t != 0) {
            if ('\n' == *t || '\r' == *t) {
                msg_ss_buf_ << std::endl;
                _flush_msg_ss_buf_nonlock();
            } else {
                msg_ss_buf_ << *t;
            }
            ++t;
        }
        return *this;
    }
    // for endl
    PilotTUI& operator<<(std::ostream& (*pf)(std::ostream&)) {
        std::lock_guard<std::mutex> lock(lock_);
        msg_ss_buf_ << std::endl;
        _flush_msg_ss_buf_nonlock();
        return *this;
    }
    PilotTUI& operator<<(const pilot_analytical_result_t &wi) {
        std::lock_guard<std::mutex> lock(lock_);
        assert (summary_box_);
        summary_box_->show_workload_info(wi);
        return *this;
    }
    PilotTUI& operator<<(pilot_analytical_result_t *wi) {
        return (*this) << const_cast<const pilot_analytical_result_t*>(wi);
    }
};

} // namespace pilot

#endif /* LIB_INTERFACE_INCLUDE_PILOT_TUI_HPP_ */
