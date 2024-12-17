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
    json.dump(data, file)

# Second case: regular data assimilation
data = copy.deepcopy(ref_data)
# Turn off data assimilation
# Rename output file
data['post_processor']['filename_prefix'] = 'case_2'
# Write the input file
with open('input_2.json',  'w') as file:
    json.dump(data, file)

# Third case: augmented data assimilation
data = copy.deepcopy(ref_data)
# Add augmented parameters
data['data_assimilation']['augment_with_beam_0_absorption'] = True
data['data_assimilation']['augment_with_beam_0_max_power'] = True
# Rename output file
data['post_processor']['filename_prefix'] = 'case_3'
# Write the input file
with open('input_3.json',  'w') as file:
    json.dump(data, file)

# Fourth case: same as previously but the conductivity in the metal is wrong
# TODO
data = copy.deepcopy(ref_data)
# Add augmented parameters
data['data_assimilation']['augment_with_beam_0_absorption'] = True
data['data_assimilation']['augment_with_beam_0_max_power'] = True
# Rename output file
data['post_processor']['filename_prefix'] = 'case_4'
# Write the input file
with open('input_4.json',  'w') as file:
    json.dump(data, file)
