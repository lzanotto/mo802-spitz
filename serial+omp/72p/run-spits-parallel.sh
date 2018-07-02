#!/bin/bash

############################################
# This is the OpenMP run for the RayTracer #
############################################

# You can either pass scenes individually as arguments or 
# leave it blank to run all scenes in the 'scenes' folder
SCENES="${@:1}"
SCENES="${SCENES:-`ls scenes/*.scn`}"

# Ray tracing parameters
export SUPER_SAMPLES=1
export DEPTH_COMPLEXITY=5
export WIDTH=128		
export HEIGHT=72
# Runtime directory
export PROGDIR="./runtime"
export PROGRAM="$PROGDIR/spitz-run.sh"

# Input binary
export MODULEDIR="./bin"
export MODULE="$MODULEDIR/RayTracer_spits.so"

# Output directory
export OUTDIR="./results/spits-parallel"

# Execution directory
export RUNDIR="./run"

# Ensure the output directory exists
mkdir -p $OUTDIR

# Ensure the execution directory exists
mkdir -p $RUNDIR

# Some PYPITS flags
#
# --overfill is the number of extra tasks sent to each Task Manager 
#            to make better use of the TCP connection, usually a 
#            value a few times larger than the number of workers 
#            works best, this also avoids starvation problems
#
export PPFLAGS="--overfill=2"

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
	    RUNSCNDIR="$RUNDIR/`basename $SCENE`"
	    echo $SCENE...
	    mkdir -p $RUNSCNDIR
	    pushd $RUNSCNDIR
	    rm -rf nodes* jm.* log
	    (time ../../$PROGRAM $PPFLAGS ../../$MODULE \
	    ../../$SCENE $SUPER_SAMPLES $DEPTH_COMPLEXITY ../../$OUTFILE $WIDTH $HEIGHT) >> ../../$OUTLOG 2>>  ../../$OUTTIME
	    popd
	done
done
