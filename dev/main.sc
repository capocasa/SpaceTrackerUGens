(
  ServerTree.removeAll;
  ServerTree.add({
    
    {
      ~buffer = Buffer.sendCollection(s, [1, 0.2, 1.5, 0.5, 1, 0.75], 2);
      s.sync;
      ~synth={
        var p;
        p=SinOsc.kr(1);
        p.poll;
      }.play;
    }.forkIfNeeded;
  });
  s.reboot;
)


