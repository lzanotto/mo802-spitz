#!/bin/bash

############################################
# This is the OpenMP run for the RayTracer #
############################################

# You can either pass scenes individually as arguments or 
# leave it blank to run all scenes in the 'scenes' folder
SCENES="${@:1}"
SCENES="${SCENES:-`ls ~/SPITZRayTracer/scenes/*.scn`}"

# Ray tracing parameters
export SUPER_SAMPLES=1
export DEPTH_COMPLEXITY=5
export WIDTH=128		
export HEIGHT=72
# Input binary
export PROGDIR="./bin"
export PROGRAM="$PROGDIR/RayTracer_spits"

# Output directory
export OUTDIR="./results/spits-serial"

# Ensure the output directory exists
mkdir -p $OUTDIR

# Iterate through selected scenes and 
# run the ray tracer using time 
for i  in `seq 1 10`;
do
	for SCENE in $SCENES
	do
	    FILENAME="$OUTDIR/`basename $SCENE`"
	    FILENAME="${FILENAME%.*}"
	    OUTFILE="$FILENAME.tga"
	    OUTLOG="$FILENAME.log"
	    OUTTIME="$FILENAME.time"
	
	    echo $SCENE...
	    (time $PROGRAM $SCENE $SUPER_SAMPLES $DEPTH_COMPLEXITY $OUTFILE $WIDTH $HEIGHT) >> $OUTLOG 2>> $OUTTIME
	done
done
