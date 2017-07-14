
TimedBufferUGens
=============

Buffer record and playback in timed buffer format, a sparse data format for stepped signals. Allows recording musical instrument data in audio rate with little memory usage.

Build
-----

1. Clone sc3-plugins 

    cd $MY_SOURCE_DIR
    git clone https://github.com/supercollider/sc3-plugins

2. Clone TimedBufferUGens:

    cd $MY_SOURCE_DIR
    git clone https://github.com/carlocapocasa/timedbufferugens.git TimedBufferUGens.git

3. Place TimedBufferUGens into your sc3-plugins without confusing git

    cd $MY_SOURCE_DIR/sc3-plugins/source
    ln -s ../../TimedBufferUGens

4. Include in sc3-plugins build script

Edit $MY_SOURCE_DIR/sc3-plugins/source/CMakeLists.txt and insert a line `TimedBufferUGens` in the `plugins without extras` list

5. Build

    mkdir $MY_SOURCE_DIR/sc3-plugins/build
    cd $MY_SOURCE_DIR/sc3-plugins/build
    
    # adapt to your system with the sc3-plugins readme
    cmake .. -DSUPERNOVA=on 
    make TimedBufferUGens JackMIDIUgens_supernova

6. Install

    sudo ln -s $MY_SOURCE_DIR/sc3-plugins/build/source/TimedBufferUGens.so /usr/lib/SuperCollider/plugins
    sudo ln -s $MY_SOURCE_DIR/sc3-plugins/build/source/TimedBufferUGens_supernova.so /usr/lib/SuperCollider/plugins 

    ln -s $MY_SOURCE_DIR/TimedBufferUGens/sc $MY_SUPERCOLLIDER_EXTENSIONS_DIR/TimedBufferUGens

Update
------

    cd $MY_SOURCE_DIR/TimedBufferUGens/build
    git pull
    make TimedBufferUGens JackMIDIUgens_supernova

    # restart server

Usage
-----

Please see the help file sc/help/PlayBufS.sc for discussion and examples

License
-------
Copyright (c) 2014 - 2017 Carlo Capocasa. Licensed under the GNU General Public License 2.0 or later.

