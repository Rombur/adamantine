#!/usr/bin/env python
# coding: utf-8

def print_header():
    return "Number of path segments\n0\nMode x y z pmod param\n"

def layer_1L2L(xmin, xmax, ymin, ymax, z, velocity, dwell, weld_on_off_dwell, interlayer_dwell_factor):
    layer_time = 0.

    os0 = "1 " + str(xmax) + " " + str(ymin) + " " + f'{z:.8f}' + " 0 " + str(weld_on_off_dwell)
    layer_time += weld_on_off_dwell

    os1 = "0 " + str(xmin) + " " + str(ymin) + " " + f'{z:.8f}' + " 1 " + str(velocity) 
    layer_time += (xmax-xmin)/velocity

    dwell_time_os4 = weld_on_off_dwell + dwell + weld_on_off_dwell
    os4 = "1 " + str(xmax) + " " + str(ymax) + " " + f'{z:.8f}' + " 0 " + str(dwell_time_os4)
    layer_time += dwell_time_os4

    os5 = "0 " + str(xmin) + " " + str(ymax) + " " + f'{z:.8f}' + " 1 " + str(velocity)
    layer_time += (xmax-xmin)/velocity

    dwell_time_os7 = weld_on_off_dwell + interlayer_dwell_factor * dwell 
    os7 = "1 " + str(xmin) + " " + str(ymax) + " " + f'{z:.8f}' + " 0 " + str(dwell_time_os7)
    layer_time += dwell_time_os7
    
    return os0 + " \n" + os1 + "\n" + os4 + "\n" + os5 + "\n" + os7 + "\n", layer_time

def layer_2R1R(xmin, xmax, ymin, ymax, z, velocity, dwell, weld_on_off_dwell, interlayer_dwell_factor):
    layer_time = 0.

    os0 = "1 " + str(xmin) + " " + str(ymax) + " " + f'{z:.8f}' + " 0 " + str(weld_on_off_dwell)
    layer_time += weld_on_off_dwell

    os1 = "0 " + str(xmax) + " " + str(ymax) + " " + f'{z:.8f}' + " 1 " + str(velocity)
    layer_time += (xmax-xmin)/velocity

    dwell_time_os4 = weld_on_off_dwell + dwell + weld_on_off_dwell
    os4 = "1 " + str(xmin) + " " + str(ymin) + " " + f'{z:.8f}' + " 0 " + str(dwell_time_os4)
    layer_time += dwell_time_os4

    os5 = "0 " + str(xmax) + " " + str(ymin) + " " + f'{z:.8f}' + " 1 " + str(velocity) 
    layer_time += (xmax-xmin)/velocity

    dwell_time_os7 = weld_on_off_dwell + interlayer_dwell_factor * dwell
    os7 = "1 " + str(xmax) + " " + str(ymin) + " " + f'{z:.8f}' + " 0 " + str(dwell_time_os7)
    layer_time += (xmax_xmin)/velocity
    
    return os0 + " \n" + os1 + "\n" + os4 + "\n" + os5 + "\n" + os7 + "\n", layer_time

def layer_1L2R(xmin, xmax, ymin, ymax, z, velocity, dwell, weld_on_off_dwell, interlayer_dwell_factor):
    layer_time = 0

    os0 = "1 " + str(xmax) + " " + str(ymin) + " " + f'{z:.8f}' + " 0 " + str(weld_on_off_dwell)
    layer_time += weld_on_off_dwell

    os1 = "0 " + str(xmin) + " " + str(ymin) + " " + f'{z:.8f}' + " 1 " + str(velocity)
    layer_time += (xmax-xmin)/velocity

    dwell_time_os4 = weld_on_off_dwell + dwell + weld_on_off_dwell
    os4 = "1 " + str(xmin) + " " + str(ymax) + " " + f'{z:.8f}' + " 0 " + str(dwell_time_os4)
    layer_time += dwell_time_os4

    os5 = "0 " + str(xmax) + " " + str(ymax) + " " + f'{z:.8f}' + " 1 " + str(velocity) 
    layer_time += (xmax-xmin)/velocity

    dwell_time_os7 = weld_on_off_dwell + interlayer_dwell_factor * dwell
    os7 = "1 " + str(xmax) + " " + str(ymax) + " " + f'{z:.8f}' + " 0 " + str(dwell_time_os7)
    layer_time += dwell_time_os7
    
    return os0 + " \n" + os1 + "\n" + os4 + "\n" + os5 + "\n" + os7 + "\n", layer_time

def layer_2L1R(xmin, xmax, ymin, ymax, z, velocity, dwell, weld_on_off_dwell, interlayer_dwell_factor):
    layer_time = 0

    os0 = "1 " + str(xmax) + " " + str(ymax) + " " + f'{z:.8f}' + " 0 " + str(weld_on_off_dwell)
    layer_time += weld_on_off_dwell

    os1 = "0 " + str(xmin) + " " + str(ymax) + " " + f'{z:.8f}' + " 1 " + str(velocity)
    layer_time += (xmax-xmin)/velocity

    dwell_time_os4 = weld_on_off_dwell + dwell + weld_on_off_dwell
    os4 = "1 " + str(xmin) + " " + str(ymin) + " " + f'{z:.8f}' + " 0 " + str(dwell_time_os4)
    layer_time += dwell_time_os4

    os5 = "0 " + str(xmax) + " " + str(ymin) + " " + f'{z:.8f}' + " 1 " + str(velocity) 
    layer_time += (xmax-xmin)/velocity

    dwell_time_os7 = weld_on_off_dwell + interlayer_dwell_factor * dwell
    os7 = "1 " + str(xmax) + " " + str(ymin) + " " + f'{z:.8f}' + " 0 " + str(dwell_time_os7)
    layer_time += dwell_time_os7
    
    return os0 + " \n" + os1 + "\n" + os4 + "\n" + os5 + "\n" + os7 + "\n", layer_time

v = 0.003871
xmin_loc = 27.0e-3
xmax_loc = 127.0e-3
ymin_loc = 4.75e-3
ymax_loc = 7.75e-3
dwell_pass = 12.0
dwell_weld_on_off = 3.0

z_substrate = 76.5e-3
layer_height = 1.5625e-3


# Build 1
#block_starts = [1, 17, 33, 49]
#total_layers = 64
#
#z = z_substrate
#running_string = print_header()
#
#for layer in range(block_starts[0], block_starts[1]):
#    out = None
#    if (layer%2 == 1):
#        out = layer_1L2L(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
#    else:
#        out = layer_2R1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
#    running_string = running_string + out
#    z = z + layer_height
#    
#for layer in range(block_starts[1], block_starts[2]):
#    out = None
#    if (layer%2 == 1):
#        out = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
#    else:
#        out = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
#    running_string = running_string + out
#    z = z + layer_height
#    
#for layer in range(block_starts[2], block_starts[3]):
#    out = None
#    if (layer%2 == 1):
#        out = layer_1L2L(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
#    else:
#        out = layer_2R1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
#    running_string = running_string + out
#    z = z + layer_height
#    
#for layer in range(block_starts[3], total_layers+1):
#    out = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
#    running_string = running_string + out
#    z = z + layer_height
#    
#f = open('build1_scan_path.txt', 'w')
#f.write(running_string)
#f.close()


# Build 2
block_starts = [1]
total_layers = 48

z = z_substrate
running_string = print_header()
time_string = ''
time = 0.

for layer in range(block_starts[0], total_layers+1):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
scan_path_file = open('build2_scan_path.txt', 'w')
scan_path_file.write(running_string)
scan_path_file.close()

time_file = open('build2_time_log.txt', 'w') 
time_file.write(time_string)
time_file.close()


# Build 3
block_starts = [1, 13, 25, 37]
total_layers = 48

z = z_substrate
running_string = print_header()
time_string = ''
time = 0.

for layer in range(block_starts[0], block_starts[1]):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
for layer in range(block_starts[1], block_starts[2]):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 5)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 5)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
for layer in range(block_starts[2], block_starts[3]):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
for layer in range(block_starts[3], total_layers+1):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 5)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 5)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
scan_path_file = open('build3_scan_path.txt', 'w')
scan_path_file.write(running_string)
scan_path_file.close()

time_file = open('build3_time_log.txt', 'w') 
time_file.write(time_string)
time_file.close()


# Build 4
block_starts = [1, 10, 16, 22, 28, 34, 40]
total_layers = 48

z = z_substrate
running_string = print_header()
time_string = ''
time = 0.

for layer in range(block_starts[0], block_starts[1]):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
for layer in range(block_starts[1], block_starts[2]):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 3)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 3)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
for layer in range(block_starts[2], block_starts[3]):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 5)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 5)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height

for layer in range(block_starts[3], block_starts[4]):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 3)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 3)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
for layer in range(block_starts[4], block_starts[5]):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 1)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
for layer in range(block_starts[5], block_starts[6]):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 3)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 3)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
for layer in range(block_starts[6], total_layers+1):
    out = None
    if (layer%2 == 1):
        out, layer_time = layer_1L2R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 5)
    else:
        out, layer_time = layer_2L1R(xmin_loc, xmax_loc, ymin_loc, ymax_loc, z, v, dwell_pass, dwell_weld_on_off, 5)
    running_string = running_string + out
    time = time + layer_time
    time_string = time_string + str(layer-1) + ',' + str(time) + ',\n'
    z = z + layer_height
    
scan_path_file = open('build4_scan_path.txt', 'w')
scan_path_file.write(running_string)
scan_path_file.close()

time_file = open('build4_time_log.txt', 'w') 
time_file.write(time_string)
time_file.close()
