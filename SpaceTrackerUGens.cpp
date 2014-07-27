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
  double m_nextphase;
  SndBuf *m_buf;
};


static void PlayST_next_k(PlayST *unit, int inNumSamples);
static void PlayST_Ctor(PlayST* unit);

void PlayST_Ctor(PlayST* unit)
{
  SETCALC(PlayST_next_k);

  unit->m_fbufnum = -1e9f;
  unit->m_phase = 0; 
  unit->m_nextphase = 0;
  unit->index = 0;

  PlayST_next_k(unit, 1);

  ClearUnitOutputs(unit, 1);

}

void PlayST_next_k(PlayST *unit, int inNumSamples)
{
  float rate     = ZIN0(1);

  GET_BUF_SHARED
  
  int numOutputs = unit->mNumOutputs;
  if (!checkBuffer(unit, bufData, bufChannels, numOutputs, inNumSamples))
    return;

  double phase = unit->m_phase;
  double nextphase = unit->m_nextphase;
  uint32 index = unit->m_index;

  // const float* frame = bufData + index * bufChannels;

  OUT(0)[0] = phase;

  phase += BUFDUR;
  
  if (phase > 0.4) {
    phase = 0;
    index++;
    if (index > 5) {
      index = 0;
    }
    printf("\n\nSPACETRACKER DEBUG\n\n %i %i %i %i %f \n\n", bufChannels, numOutputs, inNumSamples, index, bufData[0]);
  }

//  if (phase >= nextphase) {
//    //nextphase = frame[bufChannels];
//  
//    unit->m_index = index;
//    unit->m_nextphase = nextphase;
//  }
  
  unit->m_index = index;
  unit->m_phase = phase;
}

PluginLoad(PlayST)
{
    ft = inTable;
    DefineSimpleUnit(PlayST);
}

