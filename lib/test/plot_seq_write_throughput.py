#!/usr/bin/env python
#
# Plot throughput chart from the output of func_test_seq_write
#
# Copyright (c) 2017-2019 Yan Li <yanli@tuneup.ai>. All rights reserved.
# The Pilot tool and library is free software; you can redistribute it
# and/or modify it under the terms of the GNU Lesser General Public
# License version 2.1 (not any other version) as published by the Free
# Software Foundation.
#
# The Pilot tool and library is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program in file lgpl-2.1.txt; if not, see
# https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
#
# Commit 033228934e11b3f86fb0a4932b54b2aeea5c803c and before were
# released with the following license:
# Copyright (c) 2015, 2016, University of California, Santa Cruz, CA, USA.
# Created by Yan Li <yanli@tuneup.ai>,
# Department of Computer Science, Baskin School of Engineering.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Storage Systems Research Center, the
#       University of California, nor the names of its contributors
#       may be used to endorse or promote products derived from this
#       software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# REGENTS OF THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function
import sys
import math
import csv
import numpy as np
import matplotlib
matplotlib.use('PDF')
import matplotlib.pyplot as plt
## for Palatino and other serif fonts use:
#matplotlib.rc('font',**{'family':'sans-serif','sans-serif':['Palatino']})
#matplotlib.rc('font',**{'family':'sans-serif','sans-serif':['Helvetica']})
matplotlib.rc('font',**{'family':'serif','serif':['times'], 'size':20})
matplotlib.rc('text', usetex=True)

# Plot no more than this max OBD to avoid cluttering the graph.
# Note: Write BW is always aggregated no matter how many OBDs are chosen here.
MAX_OBD = 1
COLUMNS_PER_OBD = 7
# debug level: 0, 1, or 2
debug = 0

if len(sys.argv) < 3:
    print("Usage:", sys.argv[0], "input_csv output_file_prefix")
    exit(2)

file_count = 0


def plot(xdata, ydata):
    global file_count

    colors = ('b', 'g', 'r', 'c', 'm', 'y', 'k')
    line_no = 0

    if debug >= 1:
        print('Plotting file {0} (with {1} points)'.format(file_count, len(xdata)))

    color = colors[line_no % len(colors)]

    plt.plot(xdata, ydata, linestyle='-', color=color, markersize=4, label='throughput')

    plt.title('Seq. Write Throughput', fontsize=22)
    plt.xlabel('Time', fontsize=22)
    plt.ylabel('Write throughput (MB/s)', fontsize=22)

    #plt.legend(loc=4, prop={'size':22})
    plt.legend(prop={'size':22})

    # default border size is 0.1, the origin is at the left bottom
    #plt.subplots_adjust(top=1-0.06)
    #plt.subplots_adjust(bottom=0.1)
    #plt.subplots_adjust(right=1-0.04)
    #plt.subplots_adjust(left=0.11)

    plt.savefig(sys.argv[2] + str(file_count) + '.pdf', format='pdf')
    file_count += 1
    plt.clf()

tp = []
input_csv = sys.argv[1]
current_round = 0
timestamps = []
current_timestamp = 0
with open(input_csv, 'rb') as csvfile:
    csvreader = csv.reader(csvfile, delimiter=',')
    for row in csvreader:
        if row[0] == 'piid':
            continue
        try:
            if int(row[1]) != current_round:
                if len(tp) != 0:
                    plot(timestamps, tp)
                    timestamps = []
                    tp = []
                current_round = int(row[1])
                current_timestamp = 0
            else:
                current_timestamp += float(row[2])
                timestamps.append(current_timestamp)
                tp.append(float(row[3]))
        except ValueError:
            pass

plot(timestamps, tp)
