#!/usr/bin/python3

# Import
import sys
import os

# Check script argument
if len(sys.argv) != 2:
    print("Usage: ./script_name <input_file>")
    exit(1)

filename = str(sys.argv[1])

# Open file
if os.path.isfile(filename) == 0:
    print("Input file ", filename, "doesn't exist")
    exit(2)
file = open(filename, "r")

# Stock all lines
contents = file.read().splitlines()

# Close file
file.close()

# Tables contains usefull information
tables=[]

# Interpret all lines
line_number = 1
in_function = 0
for line in contents:
    if line.startswith("0000000", 0, len(line)):
        # We exit forward function
        if in_function:
            tables.append(line_number - 1)
        in_function = 0

        # Check if it is a vector function
        if line.find("vector", 0, len(line)) != -1:
            # Set that we are on function
            in_function = 1
            
            # Recup the name of the vector function
            function_name = line.split("<", len(line))[1].split(">", len(line))[0]

            # Add the name and the line where function begin
            tables.append(function_name)
            tables.append(line_number)

    # Increment line number
    line_number += 1

# Print result
for str in tables:
    print(str)
    
exit(0)
