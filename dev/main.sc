(
  ServerTree.removeAll;
  ServerTree.add({
    
    {
      ~buffer = Buffer.sendCollection(s, [1, 0.2, 1.5, 0.5, 1, 0.75], 2);
      s.sync;
      ~synth={
        PlayST.kr(1, ~buffer)
        //  .poll;
      }.play;
    }.forkIfNeeded;
  });
  s.reboot;
)


