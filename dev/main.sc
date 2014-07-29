(
  ServerTree.removeAll;
  ServerTree.add({
    
    {
      ~buffer = Buffer.sendCollection(s, 20.collect({
        arg i;
        if (i%2==0) {
          if(i%4==0){0.25}{0.5};
        }{
          0.01 * ((i-1)/2);
        };
      }).postln, 2);
      s.sync;
      ~synth={
        arg t_trig = 0, startPos = 0;
        PlayST.kr(1, ~buffer, trigger:t_trig, startPos: startPos)
          .poll;
      }.play;
    }.forkIfNeeded;
  });
  s.reboot;
)

~synth.set(\t_trig, 1, \startPos, 0);
~synth.set(\t_trig, 1, \startPos, 3);
~synth.free;

