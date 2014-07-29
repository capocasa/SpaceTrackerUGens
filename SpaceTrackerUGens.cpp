#include "SC_PlugIn.h"

static InterfaceTable *ft;

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

struct PlayST : public Unit
{
  double m_phase;
  float m_fbufnum;
  uint32 m_index;
  double m_next;
  SndBuf *m_buf;
  float m_prevtrig;
};


static void PlayST_next_k(PlayST *unit, int inNumSamples);
static void PlayST_Ctor(PlayST* unit);

void PlayST_Ctor(PlayST* unit)
{
  SETCALC(PlayST_next_k);

  unit->m_fbufnum = -1e9f;
  unit->m_phase = 0; 
  unit->m_next = 0;
  unit->m_index = 0;

  PlayST_next_k(unit, 1);

  ClearUnitOutputs(unit, 1);

}

void PlayST_next_k(PlayST *unit, int inNumSamples)
{
  float rate     = ZIN0(1);
  float trig     = ZIN0(2);

  GET_BUF_SHARED
  
  int numOutputs = unit->mNumOutputs;
  if (!checkBuffer(unit, bufData, bufChannels, numOutputs, inNumSamples))
    return;

  double phase = unit->m_phase;
  double next = unit->m_next;
  uint32 index = unit->m_index;

  const float* frame = bufData + index * bufChannels;

  for (int i = 0, j = 1; j != bufChannels; i++, j++) {
    OUT(0)[i] = frame[j];
  }

  if (trig > 0.f && unit->m_prevtrig <= 0.f) {
    phase = ZIN0(3);
    
    if (next > phase) {
      //printf("ST: trackback\n");
      while (next > phase && index >= 0) {
        //printf("ST: trackback index:%i next:%f phase:%f\n", index, next, phase);
        index--;
        next -= bufData[index*bufChannels];
      }
    } else {
      //printf("ST: catchup\n");
      // if phase==next, do nothing
      while (next < phase && index < bufFrames) {
        //printf("ST: catchup index:%i next:%f phase:%f\n", index, next, phase);
        next += bufData[index*bufChannels];
        index++;
      }
    }
  } else {

    phase += BUFDUR;

    if (phase >= next) {
      if (index < bufFrames) {
        next += frame[0];
        index++;
      }
    }
  }

  unit->m_index = index;
  unit->m_phase = phase;
  unit->m_next = next;
  unit->m_prevtrig = trig;
}

PluginLoad(PlayST)
{
    ft = inTable;
    DefineSimpleUnit(PlayST);
}

