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

