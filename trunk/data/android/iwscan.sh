#!/bin/sh

# We unpack :/data/android/iwlist to /tmp before calling
# this script in WifiDataCollector.cpp
IWLIST=/tmp/iwlist

# Get interface to scan
CARD=$1
RUNCOUNT=$2

# WifiDataCollector.cpp will read STDOUT and 
# merge the results into one list
for i in `seq 1 $RUNCOUNT`
do
	#echo $i
	echo "# Scan $i"
	$IWLIST $CARD scan
done

