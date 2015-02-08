PlayST : MultiOutUGen {
	//*ar { arg numChannels, bufnum=0, rate=1.0, trigger=1.0, startPos=0.0, loop = 0.0, doneAction=0;
	//	^this.multiNew('audio', numChannels, bufnum, rate, trigger, startPos, loop, doneAction)
	//}

	*kr { arg numChannels, bufnum=0, rate=1.0, trigger=1.0, startPos=0.0, loop = 0.0, doneAction=0;
		^this.multiNew('control', numChannels, bufnum, rate, trigger, startPos, loop, doneAction)
	}

	init { arg argNumChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(argNumChannels, rate);
	}
	argNamesInputsOffset { ^2 }
}

RecordST : UGen {
	/*
  *kr { arg inputArray, bufnum=0, offset=0.0, recLevel=1.0, preLevel=0.0,
			run=1.0, loop=1.0, trigger=1.0, doneAction=0;
		^this.multiNewList(
			['control', bufnum, offset, recLevel, preLevel, run, loop, trigger, doneAction ]
			++ inputArray.asArray
		)
	}
  */
	*kr { arg inputArray, bufnum=0, run=1.0, trigger=1.0, doneAction=0;
		^this.multiNewList(
			['control', bufnum, run, trigger, doneAction ]
			++ inputArray.asArray
		)
	}
}

