#!/bin/bash
# Tester script for assignment 1 point 9)
# Author: Mukta Darekar

# First argument is a path to a directory on the filesystem,
filesdir=$1
# Second argument is a text string which will be searched within these files
searchstr=$2

# Check if number of arguments are 2 
if [ $# -ne 2 ]
then
	echo "ERROR: Invalid number of arguments"
	echo "Total number of arguments required: 2"
	echo "1) A path to a directory on the filesystem"
	echo "2) A text string which will be searched within these files"
	exit 1

# Check if directory exists or not
elif [ ! -d "$filesdir" ] 
then
	echo "ERROR: File directory do not exist"
	echo "Confirm path and directory name"
	exit 1

# Get number of files and matching lines
else 
	# Number of files present in given directory that matches string
	#x=$(find "$filesdir" -type f | wc -l)
	x=$(grep -rl "$searchstr" "$filesdir" | wc -l)

	# Number of lines matching with given string in those files
	y=$(grep -r "$searchstr" "$filesdir" | wc -l)

	str1="The number of files are "
	str2=" and the number of matching lines are "

	# Display result
	echo $str1$x$str2$y
	exit 0
fi

#end
