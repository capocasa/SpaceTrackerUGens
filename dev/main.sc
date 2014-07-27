(
  ServerTree.removeAll;
  ServerTree.add({
    
    {
      ~buffer = Buffer.sendCollection(s, 20.collect({
        arg i;
        if (i%2==0) {
          0.5;
        }{
          0.01 * ((i-1)/2);
        };
      }).postln, 2);
      s.sync;
      ~synth={
        PlayST.kr(1, ~buffer)
        //  .poll;
      }.play;
    }.forkIfNeeded;
  });
  s.reboot;
)


