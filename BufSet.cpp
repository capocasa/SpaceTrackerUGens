struct BufSet : public Unit
{
  float m_fbufnum;
  SndBuf *m_buf;
  float m_prevtrig;
};

static void BufSet_next_k(BufTet *unit, int inNumSamples);
static void BufSet_Ctor(BufTet* unit);

void BufSet_Ctor(BufSet* unit)
{
  SETCALC(BufSet_next_k);
  unit->m_fbufnum = -1e9f;
  BufSet_next_k(unit, 1);
}
void BufSet_next_k(BufSet *unit, int inNumSamples)
{
  float trig = ZIN0(1);

  if (trig > 0.f && unit->m_prevtrig <= 0.f) {
    SIMPLE_GET_BUF
    float samplerate = ZIN0(2);
    SndBuf* buf2 = unit->mWorld->mSndBufsNonRealTimeMirror + (int)unit->m_fbufnum;
    unit->m_buf->samplerate = samplerate;
    buf2->samplerate = samplerate;
    //printf("SR %f %f\n", unit->m_buf->samplerate, buf2->samplerate);
  }

  unit->m_prevtrig = trig;
}

PluginLoad(PlayBufT)
{
  DefineSimpleUnit(PlayBufS);
}

