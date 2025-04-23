#! /usr/bin/python3

import copy
import json
import os

# Possible values are: 1, 2, 3, 1w, 2w, 3w, 2f, 3f
run_test_cases = ['2', '3', '1w', '2w', '3w', '2f', '3f']
#run_test_cases = ['1']
n_procs = 6

if run_test_cases == []:
    print("New input files will generated but test cases will not be run.")
else:
    print("We will run the following test cases", run_test_cases)

# Read the default input file
with open('reference_input.json', 'r') as file:
    ref_data = json.load(file)

##----------------------------------------------------------------------------##
## Heat Source: Goldak                                                        ##
##----------------------------------------------------------------------------##

# First case: plain simulation. Single simulation
data = copy.deepcopy(ref_data)
# Use single simulation
data['ensemble']['ensemble_simulation'] = False
# Turn off data assimilation
data['data_assimilation']['assimilate_data'] = False
# Rename output file
data['post_processor']['filename_prefix'] = 'case_1'
# Write the input file
with open('input_1.json',  'w') as file:
    json.dump(data, file, indent = 2)

# Second case: regular data assimilation
data = copy.deepcopy(ref_data)
# Rename output file
data['post_processor']['filename_prefix'] = 'case_2'
# Write the input file
with open('input_2.json',  'w') as file:
    json.dump(data, file, indent = 2)

# Third case: augmented data assimilation
# TODO extend augmented variable
data = copy.deepcopy(ref_data)
# Add augmented parameters
data['data_assimilation']['augment_with_beam_0_absorption'] = True
data['data_assimilation']['augment_with_beam_0_max_power'] = True
# Rename output file
data['post_processor']['filename_prefix'] = 'case_3'
# Write the input file
with open('input_3.json',  'w') as file:
    json.dump(data, file, indent = 2)

##----------------------------------------------------------------------------##
## Heat Source: Goldak + Wrong Conductivity                                   ##
##----------------------------------------------------------------------------##

# Do the same test cases as before but with a wrong conductivity
wrong_th_conduc = 25.0

# Fourth case: plain simulation
data = copy.deepcopy(ref_data)
# Use single simulation
data['ensemble']['ensemble_simulation'] = False
# Turn off data assimilation
data['data_assimilation']['assimilate_data'] = False
# Set the wrong conductivity
data['materials']['material_0']['solid']['thermal_conductivity_x'] = wrong_th_conduc
data['materials']['material_0']['solid']['thermal_conductivity_y'] = wrong_th_conduc
data['materials']['material_0']['solid']['thermal_conductivity_z'] = wrong_th_conduc
# Rename output file
data['post_processor']['filename_prefix'] = 'case_1_w'
# Write the input file
with open('input_1_w.json',  'w') as file:
    json.dump(data, file, indent = 2)

# Fifth case: regular data assimilation
data = copy.deepcopy(ref_data)
# Set the wrong conductivity
data['materials']['material_0']['solid']['thermal_conductivity_x'] = wrong_th_conduc
data['materials']['material_0']['solid']['thermal_conductivity_y'] = wrong_th_conduc
data['materials']['material_0']['solid']['thermal_conductivity_z'] = wrong_th_conduc
# Rename output file
data['post_processor']['filename_prefix'] = 'case_2_w'
# Write the input file
with open('input_2_w.json',  'w') as file:
    json.dump(data, file, indent = 2)

# Sixth case: augmented data assimilation
# TODO extend augmented variable
data = copy.deepcopy(ref_data)
# Set the wrong conductivity
data['materials']['material_0']['solid']['thermal_conductivity_x'] = wrong_th_conduc
data['materials']['material_0']['solid']['thermal_conductivity_y'] = wrong_th_conduc
data['materials']['material_0']['solid']['thermal_conductivity_z'] = wrong_th_conduc
# Add augmented parameters
data['data_assimilation']['augment_with_beam_0_absorption'] = True
data['data_assimilation']['augment_with_beam_0_max_power'] = True
# Rename output file
data['post_processor']['filename_prefix'] = 'case_3_w'
# Write the input file
with open('input_3_w.json',  'w') as file:
    json.dump(data, file, indent = 2)

##----------------------------------------------------------------------------##
## Heat Source: Goldak + Using More Images                                    ##
##----------------------------------------------------------------------------##

# Do the same test cases but assimilate data every 10 seconds instead of every
# 30 seconds

# Seventh case: regular data assimilation
data = copy.deepcopy(ref_data)
# Change the data time log file
data['experiment']['log_filename'] = 'observations/build_time_fine_log.txt'
# Rename output file
data['post_processor']['filename_prefix'] = 'case_2_f'
# Write the input file
with open('input_2_f.json',  'w') as file:
    json.dump(data, file, indent = 2)

# Third case: augmented data assimilation
# TODO extend augmented variable
data = copy.deepcopy(ref_data)
# Change the data time log file
data['experiment']['log_filename'] = 'observations/build_time_fine_log.txt'
# Add augmented parameters
data['data_assimilation']['augment_with_beam_0_absorption'] = True
data['data_assimilation']['augment_with_beam_0_max_power'] = True
# Rename output file
data['post_processor']['filename_prefix'] = 'case_3_f'
# Write the input file
with open('input_3_f.json',  'w') as file:
    json.dump(data, file, indent = 2)

##----------------------------------------------------------------------------##
## Run The Test Cases                                                         ##
##----------------------------------------------------------------------------##

if '1' in run_test_cases:
    print("starting test case 1")
    os.system('mpirun -np ' + str(n_procs) + ' ./adamantine -i input_1.json')
    os.system('mkdir test_case_1')
    os.system('mv case_1* test_case_1')
    os.system('tar -cf test_case_1.tar test_case_1')
    os.system('rm -r test_case_1')
    os.system('gzip test_case_1.tar')
    print("done with test case 1")
    
if '2' in run_test_cases:
    print("starting test case 2")
    os.system('mpirun -np ' + str(n_procs) + ' ./adamantine -i input_2.json')
    os.system('mkdir test_case_2')
    os.system('mv case_2* test_case_2')
    os.system('tar -cf test_case_2.tar test_case_2')
    os.system('rm -r test_case_2')
    os.system('gzip test_case_2.tar')
    print("done with test case 2")
    
if '3' in run_test_cases:
    print("starting test case 3")
    os.system('mpirun -np ' + str(n_procs) + ' ./adamantine -i input_3.json')
    os.system('mkdir test_case_3')
    os.system('mv case_3* test_case_3')
    os.system('tar -cf test_case_3.tar test_case_3')
    os.system('rm -r test_case_3')
    os.system('gzip test_case_3.tar')
    print("done with test case 3")
    
if '1w' in run_test_cases:
    print("starting test case 1w")
    os.system('mpirun -np ' + str(n_procs) + ' ./adamantine -i input_1_w.json')
    os.system('mkdir test_case_1_w')
    os.system('mv case_1_w* test_case_1_w')
    os.system('tar -cf test_case_1_w.tar test_case_1_w')
    os.system('rm -r test_case_1_w')
    os.system('gzip test_case_1_w.tar')
    print("done with test case 1w")

if '2w' in run_test_cases:
    print("starting test case 2w")
    os.system('mpirun -np ' + str(n_procs) + ' ./adamantine -i input_2_w.json')
    os.system('mkdir test_case_2_w')
    os.system('mv case_2_w* test_case_2_w')
    os.system('tar -cf test_case_2_w.tar test_case_2_w')
    os.system('rm -r test_case_2_w')
    os.system('gzip test_case_2_w.tar')
    print("done with test case 2w")

if '3w' in run_test_cases:
    print("starting test case 3w")
    os.system('mpirun -np ' + str(n_procs) + ' ./adamantine -i input_3_w.json')
    os.system('mkdir test_case_3_w')
    os.system('mv case_3_w* test_case_3_w')
    os.system('tar -cf test_case_3_w.tar test_case_3_w')
    os.system('rm -r test_case_3_w')
    os.system('gzip test_case_3_w.tar')
    print("done with test case 3w")

if '2f' in run_test_cases:
    print("starting test case 2f")
    os.system('mpirun -np ' + str(n_procs) + ' ./adamantine -i input_2_f.json')
    os.system('mkdir test_case_2_f')
    os.system('mv case_2_f* test_case_2_f')
    os.system('tar -cf test_case_2_f.tar test_case_2_f')
    os.system('rm -r test_case_2_f')
    os.system('gzip test_case_2_f.tar')
    print("done with test case 2f")

if '3f' in run_test_cases:
    print("starting test case 3f")
    os.system('mpirun -np ' + str(n_procs) + ' ./adamantine -i input_3_f.json')
    os.system('mkdir test_case_3_f')
    os.system('mv case_3_f* test_case_3_f')
    os.system('tar -cf test_case_3_f.tar test_case_3_f')
    os.system('rm -r test_case_3_f')
    os.system('gzip test_case_3_f.tar')
    print("done with test case 3f")
