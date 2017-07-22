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


FinalFrameT : UGen {
  *ar {
    thisMethod.shouldNotImplement;
  }
	*kr { arg bufnum=0;
		^this.multiNew('control', bufnum)
	}
  
  
  // RecordBufT can never record a zero pause, because
  // a trigger will always be at least one control period.
  // DetectEndS finds the first zero pause, which marks
  // then end of a recording.
  detect {
    arg buffer;
    var path, responder, id, frames, cond;
    cond = Condition.new;
    id = 262144.rand;
    path = '/finalFrameT';
    responder = OSCFunc({|msg|
      if (msg[2] == id) {
        frames = msg[3..];
        cond.test = true;
        cond.signal;
      };
    }, path);
    {
      SendReply.kr(Impulse.kr, path, DetectEndT.kr(buffer), id);
      FreeSelf.kr(Impulse.kr);
    }.play(buffer[0].server.defaultGroup);
    cond.test = false;
    cond.wait;
    responder.free;
    ^frames;
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
}

