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
  //unit->m_next = buf->data[0];
  unit->m_index = 0;

//  int bufChannels = buf->channels;
//  const float* bufData = buf->data;

  PlayST_next_k(unit, 1);

//  ClearUnitOutputs(unit, 1);

//  for (int i = 0, j = 1; j != bufChannels; i++, j++) {
//    OUT(0)[i] = bufData[j];
//  }

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

  for (int i = 0, j = 1; j < bufChannels; i++, j++) {
    OUT(i)[0] = frame[j];
  }

  //if (index < bufFrames) printf("ST: index:%i bufnum:%i bufChannels:%i BUFDUR:%f phase:%f next:%f time:%f note:%f value:%f\n", index, (int) unit->m_fbufnum, bufChannels, BUFDUR, phase, next, frame[0], frame[1], frame[2]);

  if (trig > 0.f && unit->m_prevtrig <= 0.f) {

    phase = ZIN0(3);
    
//    printf("ST: buffer dump ");
//    for (int i = 0; i < (bufFrames * bufChannels); i++) {
//      printf("%f ", bufData[i]);
//    }
//    printf("phase:%f next:%f time:%f note:%f value:%f\n", phase, next, frame[0], frame[1], frame[2]);

    if (phase == 0) {
      next = bufData[0];
      index = 0;
    } else {
      if (next > phase) {
        //printf("ST: trackback\n");
        while (next > phase && index >= 0) {
          //printf("ST: trackback index:%i next:%f phase:%f\n", index, next, phase);
          index--;
          next -= bufData[index*bufChannels];
        }
      } else {
        // if phase==next, do nothing
        //printf("ST: catchup\n");
        while (next < phase && index < bufFrames) {
          //printf("ST: catchup index:%i next:%f phase:%f\n", index, next, phase);
          next += bufData[index*bufChannels];
          index++;
        }
      }
    }
  } else {

    phase += BUFDUR;

    if (phase >= next) {
      if (index < bufFrames) {
        index++;
        next += bufData[index*bufChannels];
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

