
#Accepts the following runtime arguments: the first argument is a path to a directory on the filesystem, referred to below as filesdir; 
# the second argument is a text string which will be searched within these files, referred to below as searchstr
#
#Exits with return value 1 error and print statements if any of the parameters above were not specified
#
#Exits with return value 1 error and print statements if filesdir does not represent a directory on the filesystem
#
#Prints a message "The number of files are X and the number of matching lines are Y" where X is the number of files in the directory 
#and all subdirectories and Y is the number of matching lines found in respective files, 
#where a matching line refers to a line which contains searchstr (and may also contain additional content).
#
# Example invocation:
# finder.sh /tmp/aesd/assignment1 linux

#!/bin/bash

# Check if the required arguments are provided
if [ $# -ne 2 ]; then
    echo "Error: Please provide the path to a directory and a search string."
    exit 1
fi

# Assign the arguments to variables
filesdir=$1
searchstr=$2

echo "filesdir: $filesdir"
echo "searchstr: $searchstr"

# Check if filesdir is a directory
if [ ! -d "$filesdir" ]; then
    echo "Error: $filesdir is not a directory."
    exit 1
fi

# Find the number of files and matching lines
num_files=$(find "$filesdir" -type f | wc -l)
num_lines=$(grep -r "$searchstr" "$filesdir" | wc -l)

# Print the result
echo "The number of files are $num_files and the number of matching lines are $num_lines"
