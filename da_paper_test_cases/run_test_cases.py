#! /usr/bin/python3

import copy
import json

# Read the default input file
with open('reference_input.json', 'r') as file:
    ref_data = json.load(file)

# First case: plain simulation
data = copy.deepcopy(ref_data)
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

# Now do the same test cases as before but with a wrong conductivity
wrong_th_conduc = 25.0

# Fourth case: plain simulation
data = copy.deepcopy(ref_data)
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

# Third case: augmented data assimilation
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
