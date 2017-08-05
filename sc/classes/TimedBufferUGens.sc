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
    var frames, offset, newStartTime, newEndTime, tmp, s;
    forkIfNeeded {
      #frames, offset, newStartTime, newEndTime = this.detectTimed(startTime, length);
//[frames, offset, newStartTime, newEndTime].postln;
      this.updateInfo;
      this.server.sync;
      tmp = Buffer.alloc(this.server, frames, this.numChannels);
      this.server.sync;
      this.copyData(tmp, 0, offset*this.numChannels, frames*this.numChannels);
      this.server.sync;
      tmp.set(0, newStartTime, frames-1*this.numChannels, newEndTime);
      this.server.sync;
      //tmp.write(path, headerFormat, "float", -1, 0, false, nil);
      tmp.write(path, headerFormat, "float", frames, offset, false, nil);
      this.server.sync;
      tmp.free;
    };
  }

  // TODO: Support startTime and length
  *readTimed {
    arg server, path, startTime=0, length=0, noteLength, buf;
    var f, frames, offset, frame, time, noteLengthStart, noteLengthEnd, j, endTime;
    endTime = startTime + length;
    f = SoundFile.openRead(path);
    frame = FloatArray.newClear(f.numChannels);
    
    j = 0;
    time = 0;
    if ((startTime > 0) && (length > 0)) {
      block { |break|
        while { f.readData(frame); frame.size > 0 } {
          noteLength = frame[0];
          time = time + noteLength;
          if (noteLengthStart.isNil) {
            if (time >= startTime) {
              offset = j;
              noteLengthStart = time - startTime;
            };
          };
          if (time >= endTime) {
            break.value;
          };
          j = j + 1;
        };
      };
      noteLengthEnd = noteLength - (time - endTime).max(0);
      frames = j - offset;
    }{
      offset = 0;
      frames = f.numFrames;
    };
    f.close;

    buf=this.read(server, path, offset, frames);
    fork {
      server.sync;
      if (noteLengthStart.notNil) {
        buf.set(0, noteLengthStart, f.numChannels-1*frames, noteLengthEnd);
      };
    };
    ^buf;
  }
  readTimed {
    arg argpath, fileStartFrame = 0, numFrames = -1, bufStartFrame = 0;
    this.read(argpath, fileStartFrame, numFrames, bufStartFrame);
  }
}

