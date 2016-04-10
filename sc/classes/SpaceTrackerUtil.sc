
DetectEndS : UGen {
  *ar {
    thisMethod.shouldNotImplement;
  }
	*kr { arg bufnum=0;
		^this.multiNew('control', bufnum)
	}
}

