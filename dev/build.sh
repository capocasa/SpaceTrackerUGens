#!/bin/bash

NAME=SpaceTrackerUGens
DIR=`dirname $0`/../sc3-plugins

CMAKE="cmake -DSUPERNOVA=ON -DSC_PATH=/usr/include/SuperCollider/ .."
MAKE="make $NAME"

CWD=`pwd`

cd $DIR

if [ -e build ]; then
  cd build
else
  mkdir build
  cd build
  $CMAKE
fi

$MAKE

cd $CWD

