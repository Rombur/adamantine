#! /usr/bin/env python3

import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator
import numpy as np

colors = ['#E69F00', '#56B4E9', '#009E73', '#CC79A7']
color_coarse = colors[0]
color_fine = colors[1]
color_p2 = colors[2]
color_ideal = colors[3]

n_procs = [1, 2, 4, 8, 12]

# Coarse Mesh
#------------
time_coarse = [9266.1, 6617.7, 4647.7, 4571.9, 4825.8]
ideal_strong_time_coarse = [9266.1, 9266.1/2, 9266.1/4, 9266.1/8, 9266.1/12]

speedup_coarse = [9266.1/9266.1, 9266.1/6617.7, 9266.1/4647.7, 9266.1/4571.9, 9266.1/4825.8]
efficiency_coarse = [9266.1/9266.1, 9266.1/(6617.7*2), 9266.1/(4647.7*4),
        9266.1/(4571.9*8), 9266.1/(4825.8*12)]

min_throughput_coarse = [903.5, 1580.4, 2545.2, 2549.4, 1877.2]
max_throughput_coarse = [3958.1, 4648.5, 4705.7, 4329.8, 3935.5]
average_throughput_coarse = [1842.9, 2580.4, 3674.2, 3735.2, 3538.7]
realtime_throughput_coarse = [1538.4, 1538.4, 1538.4, 1538.4, 1538.4]
first_block_throughput_coarse = [3132.9, 4426.9, 4510.9, 4154.5, 3657.4]
last_block_throughput_coarse = [1322.0, 2000.3, 3106.2, 3422.5, 3364.4]
average_throughput_speedup_coarse = [1842.9/1842.9, 2580.4/1842.9, 3674.2/1842.9, 3735.2/1842.9, 3538.7/1842.9]
first_block_throughput_speedup_coarse = [3132.9/3132.9, 4426.9/3132.9, 4510.9/3132.9, 4154.5/3132.9,
        3657.4/3132.9]
last_block_throughput_speedup_coarse = [1322.0/1322.0, 2000.3/1322.0, 3106.2/1322.0, 3422.5/1322.0,
        3364.4/1322.0]

eval_percent_coarse = [76.24, 67.0, 70.1, 61.3, 58.6]

# Fine Mesh
#----------
time_fine = [42928.1, 24310.6, 13864.4, 9339.8, 9322.2]
ideal_strong_time_fine = [42928.1, 42928.1/2 ,42928.1/4, 42928.1/8, 42928.1/12]

speedup_fine = [42928.1/42928.1, 42928.1/24310.6, 42928.1/13864.4, 42928.1/9339.8, 42928.1/9322.2]
efficiency_fine = [42928.1/42928.1, 42928.1/(24310.6*2), 42928.1/(13864.4*4),
        42928.1/(9339.8*8), 42928.1/(9322.2*12)] 

min_throughput_fine = [1.5, 1.8, 13.5, 1.9, 1.8]
max_throughput_fine = [1264.7, 1783.9, 2669.0, 2904.3, 2923.3]
average_throughput_fine = [397.8, 702.4, 1231.7, 1828.4, 1831.8]
first_block_throughput_fine = [1124.9, 1611.5, 2434.4, 2627.7, 2581.9]
last_block_throughput_fine = [240.7, 449.2, 810.7, 1386.1, 1215.0]
average_throughput_speedup_fine = [397.8/397.8, 702.4/397.8, 1231.7/397.8, 1828.4/397.8, 1831.8/397.8]
first_block_throughput_speedup_fine = [1124.9/1124.9, 1611.5/1124.9, 2434.4/1124.9, 2627.7/1124.9, 2581.9/1124.9]
last_block_throughput_speedup_fine = [240.7/240.7, 449.2/240.7, 810.7/240.7, 1386.1/240.7, 1215.0/240.7]

eval_percent_fine = [93.6, 88.7, 87.5, 77.8, 73.5]

# P-Refinement
#-------------
time_p2 = [19202.9, 12057.6, 7153.9, 6141.0, 5926.4]
ideal_strong_time_p2 = [19202.9, 19202.9/2, 19202.9/4, 19202.9/8, 19202.9/12]

speedup_p2 = [19202.9/19202.9, 19202.9/12057.6, 19202.9/7153.9, 19202.9/6141.0,
        19202.9/5926.4]
efficiency_p2 = [19202.9/19202.9, 19202.9/(12057.6*2), 19202.9/(7153.9*4),
        19202.9/(6141.0*8), 19202.9/(5926.4*12)] 

min_throughput_p2 = [355.6, 694.8, 434.0, 1756.5, 1560.6]
max_throughput_p2 = [2303.9, 2748.1, 4230.7, 3809.2, 3770.7]
average_throughput_p2 = [889.2, 1416.2, 2387.1, 2780.8, 2881.5]
first_block_throughput_p2 = [2118.2, 2592.5, 3948.4, 3650.0, 3292.0]
last_block_throughput_p2 = [572.0, 983.5, 1733.9, 2253.9, 2475.3]
average_throughput_speedup_p2 = [889.2/889.2, 1416.2/889.2, 2387.1/889.2, 2780.8/889.2, 2881.5/889.2]
first_block_throughput_speedup_p2 = [2118.2/2118.2, 2592.5/2118.2, 3948.4/2118.2, 3650.0/2118.2, 3292.0/2118.2]
last_block_throughput_speedup_p2 = [572.0/572.0, 983.5/572.0, 1733.9/572.0, 2253.9/572.0, 2475.3/572.0]

eval_percent_fine = [87.9, 79.6, 79.6, 68.9, 64.6]

######################
# PLOT PARAMETERS
######################

plt.rcParams['xtick.labelsize'] = 18
plt.rcParams['ytick.labelsize'] = 18

######################
# TIME
######################
fig, ax = plt.subplots(figsize=(16, 12), dpi = 100)
time_coarse_plt, = plt.loglog(n_procs, time_coarse, '*-', color = color_coarse, linewidth = 3, markersize = 12,
        label='Coarse Simulation')
ideal_strong_coarse_plt, = plt.loglog(n_procs, ideal_strong_time_coarse, '--',
        color = color_ideal, linewidth = 3, markersize = 12,
        label='Ideal Strong Scaling')
time_fine_plt, = plt.loglog(n_procs, time_fine, '*-', color = color_fine, linewidth = 3, markersize = 12,
        label='H-refined Simulation')
ideal_strong_fine_plt, = plt.loglog(n_procs, ideal_strong_time_fine, '--', color
        = color_ideal, linewidth = 3, markersize = 12,
        label='Ideal Strong Scaling')
time_p2_plt, = plt.loglog(n_procs, time_p2, '*-',  color = color_p2, linewidth = 3, markersize = 12,
        label='P-refine Simulation')
ideal_strong_p2_plt, = plt.loglog(n_procs, ideal_strong_time_p2, '--', color =
        color_ideal, linewidth = 3, markersize = 12,
        label='Ideal Strong Scaling')
ax.legend(handles=[time_coarse_plt, time_fine_plt, time_p2_plt, ideal_strong_coarse_plt], fontsize = 18)
plt.xlabel('Number of cores', fontsize = 20)
plt.ylabel('Time (s)', fontsize = 20)
ax.grid(which='both')
plt.savefig('dwell_rook_time.png')
#plt.show()
plt.clf()

######################
# SPEEDUP
#####################
fig, ax = plt.subplots(figsize=(16, 12), dpi = 100)
speedup_coarse_plt, = plt.plot(n_procs, speedup_coarse, '-', color =
        color_coarse, linewidth = 3, markersize = 12,
        label='Coarse Simulation')
speedup_fine_plt, = plt.plot(n_procs, speedup_fine, '-', color =
        color_fine, linewidth = 3, markersize = 12,
        label='H-refined Simulation')
speedup_p2_plt, = plt.plot(n_procs, speedup_p2, '-', color =
        color_p2, linewidth = 3, markersize = 12,
        label='P-refined Simulation')
ideal_speedup_plt, = plt.plot(n_procs, n_procs , '-', color = color_ideal, linewidth = 3, markersize = 12,
        label='Ideal Scaling')
ax.legend(handles=[speedup_coarse_plt, speedup_fine_plt, speedup_p2_plt,  ideal_speedup_plt], fontsize = 18)
plt.xlabel('Number of cores', fontsize = 20)
plt.ylabel('Speedup', fontsize = 20)
ax.grid(which='both')
plt.savefig('dwell_rook_speedup.png')
#plt.show()
plt.clf()

######################
# THROUGHPUT
######################
fig, ax = plt.subplots(figsize=(16, 12), dpi = 100)

avg_coarse_plt, = plt.plot(n_procs, average_throughput_coarse, '-', color =
        color_coarse, linewidth = 3, markersize = 12,
        label='Average: Coarse')
first_coarse_plt, = plt.plot(n_procs, first_block_throughput_coarse, '-.', color
        = color_coarse, linewidth = 3, 
        markersize = 12, label='First Block: Coarse')
last_coarse_plt, = plt.plot(n_procs, last_block_throughput_coarse, '--', color =
        color_coarse, linewidth = 3, 
        markersize = 12, label='Last Block: Coarse')

avg_fine_plt, = plt.plot(n_procs, average_throughput_fine, '-', color =
        color_fine, linewidth = 3, markersize = 12,
        label='Average: H-refine')
first_fine_plt, = plt.plot(n_procs, first_block_throughput_fine, '-.', color =
        color_fine, linewidth = 3, 
        markersize = 12, label='First Block: H-refine')
last_fine_plt, = plt.plot(n_procs, last_block_throughput_fine, '--', color =
        color_fine, linewidth = 3, 
        markersize = 12, label='Last Block: H-refine')

avg_p2_plt, = plt.plot(n_procs, average_throughput_p2, '-', color =
        color_p2, linewidth = 3, markersize = 12,
        label='Average: P-refine')
first_p2_plt, = plt.plot(n_procs, first_block_throughput_p2, '-.',
        color = color_p2, linewidth = 3, 
        markersize = 12, label='First Block: P-refine')
last_p2_plt, = plt.plot(n_procs, last_block_throughput_p2, '--', color
        = color_p2, linewidth = 3, 
        markersize = 12, label='Last Block: P-refine')

realtime_coarse_plt, = plt.plot(n_procs, realtime_throughput_coarse, ':', color
        = color_ideal, linewidth = 3, markersize = 12, label='Real Time')
ax.legend(handles=[first_coarse_plt, avg_coarse_plt, last_coarse_plt,
    first_fine_plt, avg_fine_plt, last_fine_plt, first_p2_plt, avg_p2_plt,
    last_p2_plt, realtime_coarse_plt], ncols = 2, fontsize = 18)
#plt.title('Throughput' , fontsize = 20)
plt.xlabel('Number of cores', fontsize = 20)
plt.ylabel('Iterations/s', fontsize = 20)
plt.xlim(1, 12)
plt.ylim(0, 4800)
ax.xaxis.set_major_locator(MultipleLocator(1))
ax.yaxis.set_major_locator(MultipleLocator(500))
ax.grid(True)
plt.savefig('dwell_rook_throughput.png')
#plt.show()
plt.clf()

######################
# THROUGHPUT SPEEDUP
######################
fig, ax = plt.subplots(figsize=(16, 12), dpi = 100)

avg_speedup_coarse_plt, = plt.plot(n_procs, average_throughput_speedup_coarse, '-', color =
        color_coarse, linewidth = 3, markersize = 12,
        label='Average: Coarse')
first_speedup_coarse_plt, = plt.plot(n_procs, first_block_throughput_speedup_coarse, '-.', color
        = color_coarse, linewidth = 3, 
        markersize = 12, label='First Block: Coarse')
last_speedup_coarse_plt, = plt.plot(n_procs, last_block_throughput_speedup_coarse, '--', color =
        color_coarse, linewidth = 3, 
        markersize = 12, label='Last Block: Coarse')

avg_speedup_fine_plt, = plt.plot(n_procs, average_throughput_speedup_fine, '-', color =
        color_fine, linewidth = 3, markersize = 12,
        label='Average: H-refine')
first_speedup_fine_plt, = plt.plot(n_procs, first_block_throughput_speedup_fine, '-.', color =
        color_fine, linewidth = 3, 
        markersize = 12, label='First Block: H-refine')
last_speedup_fine_plt, = plt.plot(n_procs, last_block_throughput_speedup_fine, '--', color =
        color_fine, linewidth = 3, 
        markersize = 12, label='Last Block: H-refine')

avg_speedup_p2_plt, = plt.plot(n_procs, average_throughput_speedup_p2, '-', color =
        color_p2, linewidth = 3, markersize = 12,
        label='Average: P-refine')
first_speedup_p2_plt, = plt.plot(n_procs, first_block_throughput_speedup_p2, '-.',
        color = color_p2, linewidth = 3, 
        markersize = 12, label='First Block: P-refine')
last_speedup_p2_plt, = plt.plot(n_procs, last_block_throughput_speedup_p2, '--', color
        = color_p2, linewidth = 3, 
        markersize = 12, label='Last Block: P-refine')
ideal_speedup_plt, = plt.plot(n_procs, n_procs , '-', color = color_ideal, linewidth = 3, markersize = 12,
        label='Ideal Scaling')

ax.legend(handles=[first_speedup_coarse_plt, avg_speedup_coarse_plt, last_speedup_coarse_plt,
    first_speedup_fine_plt, avg_speedup_fine_plt, last_speedup_fine_plt, first_speedup_p2_plt, avg_speedup_p2_plt,
    last_speedup_p2_plt, ideal_speedup_plt],  ncols = 2, fontsize = 18)
#plt.title('Throughput' , fontsize = 20)
plt.xlabel('Number of cores', fontsize = 20)
plt.ylabel('Speedup', fontsize = 20)
plt.xlim(1, 12)
#plt.ylim(0, 4800)
ax.xaxis.set_major_locator(MultipleLocator(1))
ax.yaxis.set_major_locator(MultipleLocator(1))
ax.grid(True)
plt.savefig('dwell_rook_throughput_speedup.png')
#plt.show()
plt.clf()
