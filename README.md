
SpaceTrackerUGens
=============

Record and playback musical instrument data on the server in timed buffer format, a sparse data format for stepped signals. Allows recording in audio rate with little memory usage.

Build
-----

Clone sc3-plugins 

    cd $MY_SOURCE_DIR
    git clone https://github.com/supercollider/sc3-plugins

Clone SpaceTrackerUGens:

    cd $MY_SOURCE_DIR
    git clone https://github.com/carlocapocasa/SpaceTrackerUGens.git

Place SpaceTrackerUGens into your sc3-plugins without confusing git

    cd $MY_SOURCE_DIR/sc3-plugins/source
    ln -s ../../SpaceTrackerUGens

Include in sc3-plugins build script

Edit `$MY_SOURCE_DIR/sc3-plugins/source/CMakeLists.txt` and insert a line `SpaceTrackerUGens` in the section `plugins without extras`

Build

    mkdir $MY_SOURCE_DIR/sc3-plugins/build
    cd $MY_SOURCE_DIR/sc3-plugins/build
    
    # adapt to your system with the sc3-plugins readme
    cmake .. -DSUPERNOVA=on 
    make SpaceTrackerUGens JackMIDIUgens_supernova

Install

    sudo ln -s $MY_SOURCE_DIR/sc3-plugins/build/source/SpaceTrackerUGens.so /usr/lib/SuperCollider/plugins
    sudo ln -s $MY_SOURCE_DIR/sc3-plugins/build/source/SpaceTrackerUGens_supernova.so /usr/lib/SuperCollider/plugins 

    ln -s $MY_SOURCE_DIR/SpaceTrackerUGens/sc $MY_SUPERCOLLIDER_EXTENSIONS_DIR/SpaceTrackerUGens

Update
------

    cd $MY_SOURCE_DIR/SpaceTrackerUGens/build
    git pull
    make SpaceTrackerUGens JackMIDIUgens_supernova

    # restart server

Usage
-----

Please see the help file sc/help/PlayBufT.sc for instructions and examples

License
-------
Copyright (c) 2014 - 2017 Carlo Capocasa. Licensed under the GNU General Public License 2.0 or later.

