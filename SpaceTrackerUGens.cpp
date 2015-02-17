#include "SC_PlugIn.h"

static InterfaceTable *ft;

// For RecordST
// from server/plugins/DelayUGens.cpp
// keep in sync manually
#define TAKEDOWN_IN \
  if(unit->mIn){ \
    RTFree(unit->mWorld, unit->mIn); \
  }

#define CHECK_BUF \
  if (!bufData) { \
                unit->mDone = true; \
    ClearUnitOutputs(unit, inNumSamples); \
    return; \
  }
#define SETUP_IN(offset) \
  uint32 numInputs = unit->mNumInputs - (uint32)offset; \
  if (numInputs != bufChannels) { \
    if(unit->mWorld->mVerbosity > -1 && !unit->mDone){ \
      Print("buffer-writing UGen channel mismatch: numInputs %i, yet buffer has %i channels\n", numInputs, bufChannels); \
    } \
    unit->mDone = true; \
    ClearUnitOutputs(unit, inNumSamples); \
    return; \
  } \
  if(!unit->mIn){ \
    unit->mIn = (float**)RTAlloc(unit->mWorld, numInputs * sizeof(float*)); \
    if (unit->mIn == NULL) { \
      unit->mDone = true; \
      ClearUnitOutputs(unit, inNumSamples); \
      return; \
    } \
  } \
  float **in = unit->mIn; \
  for (uint32 i=0; i<numInputs; ++i) { \
    in[i] = ZIN(i+offset); \
  }
// end server/plugins/DelayUGens.cpp

// For PlayST
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

struct RecordST : public Unit
{
  int32 m_writepos;
  float **mIn;
  float m_fbufnum;
  SndBuf *m_buf;
  float m_previnval;
  double m_phase;
  double m_lastphase;
};

static void PlayST_next_k(PlayST *unit, int inNumSamples);
static void PlayST_Ctor(PlayST* unit);

static void RecordST_next_k(RecordST *unit, int inNumSamples);
static void RecordST_Ctor(RecordST *unit);

void PlayST_Ctor(PlayST* unit)
{
  SETCALC(PlayST_next_k);
  
  GET_BUF_SHARED
  if (!checkBuffer(unit, bufData, bufChannels, unit->mNumOutputs, 1))
    return;

  unit->m_fbufnum = -1e9f;
  unit->m_phase = 0; 
  //unit->m_next = 0;
  unit->m_next = buf->data[0];
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

  int silentFrame = 0;

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
        if (next < phase && index < bufFrames) {
          unit->mDone = false;
        }
      }
    }

  } else {

    phase += BUFDUR * rate;

    if (phase >= next) {
      if (index < bufFrames) {
        index++;
        next += bufData[index*bufChannels];
        silentFrame = 1;
      } else {
        unit->mDone = true;
      }
    }
  }
  
  for (int i = 0, j = 1; j < bufChannels; i++, j++) {
    if (rate > 0 && silentFrame == 0) {
      OUT(i)[0] = frame[j];
    } else {
      OUT(i)[0] = 0;
    }
  }
  
  if (unit->mDone) {
    DoneAction(IN0(5), unit);
  }

  unit->m_index = index;
  unit->m_phase = phase;
  unit->m_next = next;
  unit->m_prevtrig = trig;
}

void RecordST_Ctor(RecordST *unit)
{

  unit->mIn = 0;
  unit->m_writepos = 0;
  unit->m_previnval = 0.f;

  unit->m_phase = 0;
  unit->m_lastphase = 0;

  SETCALC(RecordST_next_k);

  ClearUnitOutputs(unit, 1);
}
  
void RecordST_Dtor(RecordST *unit)
{
  TAKEDOWN_IN
}

void RecordST_next_k(RecordST *unit, int inNumSamples)
{

  GET_BUF
  CHECK_BUF
  SETUP_IN(3)

  float run     = ZIN0(1);
  float inval     = *++(in[0]);
  int32 writepos = unit->m_writepos;
  double phase = unit->m_phase;
  double lastphase = unit->m_lastphase;

  float* table0 = bufData + writepos;
  
//  table0[0] = *++(in[0]);


//  if (writepos > bufSamples) {   
//    writepos = 0;
//    printf("RecordST: writepos: %i; run: %f; bufSamples: %i, fbufnum: %f, inval: %f\n", writepos, run, bufSamples, fbufnum, inval);
//  }

  if (inval != unit->m_previnval) {
    printf("change\n");
    
    table0[0] = inval;
    table0[1] = phase - lastphase;
    writepos += 2;
  
    if (writepos > bufSamples) {
      unit->mDone = true;
      DoneAction(IN0(2), unit);
    }

    printf("RecordST: wrote inval %f length %f to writepos %i", phase, lastphase, writepos);
  }

  lastphase = phase;
  phase += BUFDUR;

  unit->m_writepos = writepos;
  unit->m_previnval = inval;
  unit->m_phase = phase;
  unit->m_lastphase = lastphase;
}

PluginLoad(PlayST)
{
    ft = inTable;
    DefineSimpleUnit(PlayST);
    DefineDtorUnit(RecordST);
}

