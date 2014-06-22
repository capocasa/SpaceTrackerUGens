#include "SC_PlugIn.h"

static InterfaceTable *ft;

// UTILITY

// These were copy/pasted from DelayUGens.cpp for no other
// reason than that it was easy. Presumably there is a
// better way to do this, but until finding it these
// should be probably manually updated be kept in sync
// with those from the source.

static inline bool checkBuffer(Unit * unit, const float * bufData, uint32 bufChannels,
                 uint32 expectedChannels, int inNumSamples)
{
  if (!bufData)
    goto handle_failure;

  if (expectedChannels > bufChannels) {
    if(unit->mWorld->mVerbosity > -1 && !unit->mDone)
      Print("Buffer UGen channel mismatch: expected %i, yet buffer has %i channels\n",
          expectedChannels, bufChannels);
    goto handle_failure;
  }
  return true;

handle_failure:
  unit->mDone = true;
  ClearUnitOutputs(unit, inNumSamples);
  return false;
}


inline double sc_loop(Unit *unit, double in, double hi, int loop)
{
  // avoid the divide if possible
  if (in >= hi) {
    if (!loop) {
      unit->mDone = true;
      return hi;
    }
    in -= hi;
    if (in < hi) return in;
  } else if (in < 0.) {
    if (!loop) {
      unit->mDone = true;
      return 0.;
    }
    in += hi;
    if (in >= 0.) return in;
  } else return in;

  return in - hi * floor(in/hi);
}

// MAIN

struct PlayST : public Unit
{
  double m_phase;
  float m_prevtrig;
  float m_fbufnum;
  int32 m_index;
  SndBuf *m_buf;
};

static void PlayST_next_k(PlayST *unit, int inNumSamples);
static void PlayST_Ctor(PlayST* unit);

void PlayST_Ctor(PlayST* unit)
{
  SETCALC(PlayST_next_k);
  PlayST_next_k(unit, 1);

  unit->m_fbufnum = -1e9f;
  unit->m_prevtrig = 0.;
  unit->m_phase = ZIN0(3);

  unit->m_index = 0; // TODO: init like seek

  //ClearUnitOutputs(unit, 1);
}

void PlayST_next_k(PlayST *unit, int inNumSamples)
{
  float rate     = ZIN0(1);
  // float trig     = ZIN0(2);
  // int32 loop     = (int32)ZIN0(4);

  GET_BUF_SHARED
  int numOutputs = unit->mNumOutputs;
  if (!checkBuffer(unit, bufData, bufChannels, numOutputs, inNumSamples))
    return;

  //double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
  double phase = unit->m_phase;

  // TODO: implement seek
  //if (trig > 0.f && unit->m_prevtrig <= 0.f) {
  //  unit->mDone = false;
  //  phase = ZIN0(3);
  //}
  //unit->m_prevtrig = trig;
  for (int i=0; i<inNumSamples; ++i) {
    //phase = sc_loop((Unit*)unit, phase, loopMax, loop); \
            
    int32 iphase = (int32)phase;
    const float* table1 = bufData + iphase * bufChannels;
    int32 index = 0;
    for (uint32 channel=0; channel<numOutputs; ++channel) {
      OUT(channel)[index] = table1[index++];
    }

    phase += rate;
  }
  if(unit->mDone)
    DoneAction((int)ZIN0(5), unit);
  unit->m_phase = phase;
}


