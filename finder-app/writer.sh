#!/bin/bash
# Tester script for assignment 1 point 10)
# Author: Mukta Darekar

# First argument is a path to a directory with filename where file needs to be stored,
writefile=$1
# Second argument is a text string which will be written within this file
writestr=$2

# Check if number of arguments are 2 
if [ $# -ne 2 ]
then
	echo "ERROR: Invalid number of arguments"
	echo "Total number of arguments required: 2"
	echo "1) A full path to a file (including filename) on the filesystem"
	echo "2) A text string which will be written within this file,"
	exit 1
fi

# Create directory and file if doesn't exist
if [ ! -d "${writefile%/*}" ] 
then
	mkdir -p "${writefile%/*}"
	touch $writefile
fi

# Copy given string to created / existing file
echo $writestr > $writefile


# Check if file got created
if [ ! -f "$writefile" ] 
then
	echo "ERROR: File do not exist, file creation error"
	exit 1
	
# Check if file got updated
else 
	# Number of lines matching with given string in those files
	y=$(grep -r "$writestr" "$writefile" * | wc -l)

	if [ $y -eq 1 ]
	then
		echo "File written successfully"
		exit 0
	else
		echo "ERROR: File didn't get updated"
		exit 1
	fi
fi


