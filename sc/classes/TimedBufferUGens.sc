PlayBufT : MultiOutUGen {

	*ar { arg numChannels, bufnum=0, rate=1.0, trigger=1.0, startPos=0.0, loop=0.0, doneAction=0;
		^this.multiNew('audio', numChannels, bufnum, rate, trigger, startPos, loop, doneAction)
  }

	*kr { arg numChannels, bufnum=0, rate=1.0, trigger=1.0, startPos=0.0, loop=0.0, doneAction=0;
		^this.multiNew('control', numChannels, bufnum, rate, trigger, startPos, loop, doneAction)
	}

	init { arg argNumChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(argNumChannels, rate);
	}
	argNamesInputsOffset { ^2 }
}

RecordBufT : UGen {
	*ar { arg inputArray, bufnum=0, run=1.0, doneAction=0;
		^this.multiNewList(
			if (inputArray.first.isArray == false) {
        inputArray = [inputArray];
      };
      ['audio', bufnum, run, doneAction ]
			++ inputArray.asArray.multiChannelExpand
		)
	}
	*kr { arg inputArray, bufnum=0, run=1.0, doneAction=0;
		^this.multiNewList(
			if (inputArray.first.isArray == false) {
        inputArray = [inputArray];
      };
			['control', bufnum, run, doneAction ]
			++ inputArray.asArray.multiChannelExpand
		)
	}
}

IndexBufT : UGen {
  *ar {
    thisMethod.notYetImplemented;
  }
	*kr { arg bufnum=0, trigger=1.0, startPos=0.0, controlDurTrunc = 0.0;
		^this.multiNew('control', bufnum, trigger, startPos, controlDurTrunc)
	}
}


// RecordBufT can never record a zero pause, because
// a trigger will always be at least one control period.
// BufFramesT finds the first zero pause, which marks
// the end of a recording.

BufFramesT : MultiOutUGen {
  *ar {
    thisMethod.shouldNotImplement;
  }
	*kr { arg bufnum=0, trig = 1, startTime=0, length=0, doneAction=0;
		^this.multiNew('control', bufnum, trig, startTime, length, doneAction)
	}
	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(4, rate);
	}
}

+ Buffer {
  *allocTimed {
    arg server, polyphony=1, numChannels=1, frames = 16384;
    if (polyphony == 1) {
      ^Buffer.alloc(server, frames, numChannels + 1);
    };
    ^polyphony.collect{Buffer.alloc(server, frames, numChannels + 1)};
  }
  
  detectTimed {
    arg startTime, length;
    var path, responder, id, detected, cond;
    cond = Condition.new;
    id = 262144.rand;
    path = '/finalFrameT';
    responder = OSCFunc({|msg|
      if (msg[2] == id) {
        detected = msg[3..];
        cond.test = true;
        cond.signal;
      };
    }, path);
    {
      SendReply.kr(DC.kr(1), path, BufFramesT.kr(this, 1, startTime, length, 2), id);
    }.play(this.server.defaultGroup);
    cond.test = false;
    cond.wait;
    responder.free;
    ^detected;
  }

  writeTimed {
    arg path, headerFormat="aiff", startTime=0, length=0;
    var frames, offset, noteLengthStart, noteLengthEnd, tmp, s;
    forkIfNeeded {
      #frames, offset, noteLengthStart, noteLengthEnd = this.detectTimed(startTime, length);
//[frames, offset, noteLengthStart, noteLengthEnd].postln;
      this.updateInfo;
      this.server.sync;
      tmp = Buffer.alloc(this.server, frames, this.numChannels);
      this.server.sync;
//this.server.sync;tmp.updateInfo;[\detectTimed, frames, offset, noteLengthStart, noteLengthEnd].postln;
      this.copyData(tmp, 0, offset, frames);
//this.server.sync;tmp.getn(0,frames*this.numChannels,{|c|(["pre"]++c).postln});
      this.server.sync;
      if (noteLengthStart > 0) {
        if (noteLengthEnd > 0) {
          tmp.set(0, noteLengthStart, frames-1*this.numChannels, noteLengthEnd);
        }{
          tmp.set(0, noteLengthStart);
        };
      }{
        if (noteLengthEnd > 0) {
          tmp.set(frames-1*this.numChannels, noteLengthEnd);
        };
      };
//this.server.sync;tmp.getn(0,frames*this.numChannels,{|c|(["post"]++c).postln});
      this.server.sync;
      tmp.write(path, headerFormat, "float", -1, 0, false, nil);
      this.server.sync;
      tmp.free;
    };
  }

  *readTimed {
    arg server, path, startTime=0, length=0;
    var buf, f, frames=0, offset=0, frame, endTime=0,
    
    lastlength=0,noteLength=0,time=0,lasttime=0,nexttime=0,pre=0,post=0,preassigned=false;
 
    if (length > 0) {
      endTime = startTime + length;
    };
    f = SoundFile.openRead(path);
    frame = FloatArray.newClear(f.numChannels);
    
    block { |break|
      while { f.readData(frame); frame.size > 0 } {

        // Note: This is a transliteration of the algorithm
        // in TimedBufferUGens.cpp in BufFramesT_next that is unit
        // tested. It needs to be manually kept in sync

        lastlength = noteLength;
        noteLength = frame[0];
        nexttime = time + noteLength;
        if (noteLength == 0) {
          break.value;
        };
//\foo.postln;
        if (startTime == 0) {
          frames = frames + 1;
        } {
          if (nexttime > startTime) {
             frames = frames + 1;
          }{
            offset = offset + 1;
          };
        };
//\bar.postln;

        if ((startTime > 0) && (nexttime >= startTime)) {
          if (preassigned == false) {
            pre = nexttime - startTime;
            preassigned = true;
          };
        };

//[\fuz,nexttime,endTime].postln;
        if ((endTime > 0) && (nexttime > endTime)) {
          post = noteLength - (endTime - time);
          if (time < startTime) { // corner case: within last note
            post = pre = endTime - startTime;
          };

          break.value;
        };
        if (nexttime == endTime) {
          break.value;
        };
        lasttime = time;
        time = nexttime; 
      };
    };
 
    f.close;

//[frames,offset,pre, post].postln;

    buf=this.read(server, path, offset, frames);
    fork {
      server.sync;
      if (pre>0) {
        if (post > 0) {
          buf.set(0, pre, f.numChannels-1*frames, post);
        }{
          buf.set(0, pre);
        };
      }{
        if (post > 0) {
          buf.set(f.numChannels-1*frames, post);
        };
      };
    };
    ^buf;
  }
  readTimed {
    arg argpath, fileStartFrame = 0, numFrames = -1, bufStartFrame = 0;
    this.read(argpath, fileStartFrame, numFrames, bufStartFrame);
  }
}

