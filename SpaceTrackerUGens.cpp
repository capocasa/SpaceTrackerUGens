#include "SC_PlugIn.h"
#include <iostream>

static InterfaceTable *ft;

// For RecordBufS
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

// Modified SETUP_IN from DelayUGens.cpp
// Since once channel of the buffer is used to keep track
// of times, it requires one more channel in the buffer
// than the number of inputs
// These needs to be manually kept in sync for new SC versions
// and the modification added

#define SETUP_IN_ST(offset) \
  uint32 numInputs = unit->mNumInputs - (uint32)offset; \
  if ((numInputs + 1) != bufChannels) { \
    if(unit->mWorld->mVerbosity > -1 && !unit->mDone){ \
      Print("RecordBufS channel mismatch: numInputs %i, yet buffer has %i channels. buffer needs one more channel than numInputs to store time\n", numInputs, bufChannels); \
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

// Direct copy from DelayUGens.cpp, update manually
// Potentially use for looping

/*
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
*/


// Copied from DelayUGens.cpp and modified
// Must be manually kept in sync and adapted
// to ST channel numbers
// (ST has one less output than buffer channels, because the first
//  channel of the buffer is for times)
static inline bool checkBufferST(Unit * unit, const float * bufData, uint32 bufChannels,
                 uint32 numOutputs, int inNumSamples)
{
  if (!bufData)
    goto handle_failure;

  if (numOutputs + 1 != bufChannels) {
    if(unit->mWorld->mVerbosity > -1 && !unit->mDone)
      Print("ST UGen channel mismatch: numOutputs %i, yet buffer has %i channels. buffer needs one more channel than numInputs to store time\n", numOutputs, bufChannels);
    goto handle_failure;
  }
  return true;

handle_failure:
  unit->mDone = true;
  ClearUnitOutputs(unit, inNumSamples);
  return false;
}

struct PlayBufS : public Unit
{
  double m_phase;
  float m_fbufnum;
  uint32 m_index;
  double m_next;
  SndBuf *m_buf;
  float m_prevtrig;
  float m_prevbufnum;
};

struct RecordBufS : public Unit
{
  int32 m_writepos;
  float **mIn;
  float m_fbufnum;
  SndBuf *m_buf;
  float m_previnval;
  double m_phase;
  double m_lastphase;
};

struct PlayBufSIndex : public Unit
{
  float m_fbufnum;
  SndBuf *m_buf;
  float m_prevtrig;
  double m_val;
};

struct BufSet : public Unit
{
  float m_fbufnum;
  SndBuf *m_buf;
  float m_prevtrig;
};

struct DetectEndS : public Unit
{
  float m_fbufnum;
  SndBuf *m_buf;
};

static void PlayBufS_next(PlayBufS *unit, int inNumSamples);
static void PlayBufS_Ctor(PlayBufS* unit);

static void RecordBuf_next(RecordBufS *unit, int inNumSamples);
static void RecordBufS_Ctor(RecordBufS *unit);

static void PlayBufSIndex_next_k(PlayBufSIndex *unit, int inNumSamples);
static void PlayBufSIndex_Ctor(PlayBufSIndex* unit);

static void BufSet_next_k(BufSet *unit, int inNumSamples);
static void BufSet_Ctor(BufSet* unit);

static void DetectEndS_next_k(DetectEndS *unit, int inNumSamples);
static void DetectEndS_Ctor(DetectEndS* unit);

void PlayBufS_Ctor(PlayBufS* unit)
{
  SETCALC(PlayBufS_next);

  unit->m_fbufnum = -1e9f;
  unit->m_prevbufnum = -1e9f;
  unit->m_phase = 0; 
  unit->m_next = 0;
  unit->m_index = 0;
  unit->m_prevtrig = 0;

  //PlayBufS_next(unit, 1);

  // std::cout.precision(17);

}

void PlayBufS_next(PlayBufS *unit, int inNumSamples)
{
  GET_BUF_SHARED
  
  int numOutputs = unit->mNumOutputs;
  if (!checkBufferST(unit, bufData, bufChannels, numOutputs, inNumSamples))
    return;

  double phase = unit->m_phase;
  double next = unit->m_next;
  uint32 index = unit->m_index;
  float bufnum = unit->m_fbufnum;
  float prevbufnum = unit->m_prevbufnum;

  const float* frame;

  int done = unit->mDone;

  bool audiorate = inNumSamples != 1;

  double phase_increment;
  if (audiorate) {
    phase_increment = SAMPLEDUR;
  } else {
    phase_increment = BUFDUR;
  }

  float rate;
  float trig;
  
  rate     = IN(1)[0];
  trig     = IN(2)[0];
  
  //std::cout << "rate: " << IN(1)[0] << " trig:" << IN(2)[0] << "\n";

  int x;
  for (x = 0; x < inNumSamples; x++) {
  

  
    //std::cout << "frame audiorate:" << audiorate << " x:" << x << " phase:" << std::fixed << phase << " phase_increment:" << std::fixed << phase_increment << " rate:" << rate <<" r*pi: " << std::fixed << (rate * phase_increment) << "index:"<<index<<" done:" << done << "\n";


    //if (index < bufFrames) printf("ST: index:%i bufnum:%i bufChannels:%i BUFDUR:%f phase:%f next:%f time:%f note:%f value:%f\n", index, (int) unit->m_fbufnum, bufChannels, BUFDUR, phase, next, frame[0], frame[1], frame[2]);

    if (prevbufnum != bufnum) {
      next = bufData[0];
      if (next == 0) {
        next = 16777215;
      }
      //printf("PlayBufS: initialized. rate:%f phase:%f next: %f\n bufnum:%f prevbufnum:%f\n", rate, phase, next, bufnum, prevbufnum);
    }
    unit->m_prevbufnum = bufnum;

    // Respont to triggers instantly, before output
    
    if (trig > 0.f && unit->m_prevtrig <= 0.f) {

      phase = IN(3)[x];
      
      //printf("PlayBufS: triggered. phase:%f next:%f time:%f bufnum:%f note:%f value:%f\n", phase, next, unit->m_fbufnum, frame[0], frame[1], frame[2]);
      
  //    printf("ST: buffer dump ");
  //    for (int i = 0; i < (bufFrames * bufChannels); i++) {
  //      printf("%f ", bufData[i]);
  //    }

      done = false;
      
      if (phase <= 0) {
        next = bufData[0];
        index = 0;
        phase = 0;
        if (next == 0) {
          next = 16777215;
        }
      } else {
        if (next > phase) {
          double prevnext;
          while (true) {
            //printf("PlayBufS: trackback index:%i next:%f phase:%f\n", index, next, phase);
            prevnext = next;
            next -= bufData[index*bufChannels];
            
            if (next <= phase) {
              next = prevnext;
              break;
            }

            index--;
          }
          //printf("PlayBufS: trackbacked. index:%i next:%f phase:%f\n", index, next, phase);
        } else {
          // if phase==next, do nothing
          while (next < phase) {
            //printf("PlayBufS: catchup index:%i next:%f phase:%f\n", index, next, phase);
            index++;
            if (index >= bufFrames) {
              done = true;
              phase = next;
              index = bufFrames - 1;
              break;
            }
            next += bufData[index*bufChannels];
          }
          //printf("PlayBufS: caught up. index:%i next:%f phase:%f\n", index, next, phase);
        }
      }

    }

    frame = bufData + index * bufChannels;
    
    // debug output
    //printf("out %f %f - %i %i %i %f - %f . %f %f %f\n", rate, phase, index, bufFrames, done, trig, frame[0], frame[1], frame[2], frame[3]);

    for (int i = 0, j = 1; j < bufChannels; i++, j++) {
      if (rate > 0) {
        OUT(i)[0] = frame[j];
      } else {
        OUT(i)[0] = 0;
      }
    }
    
    // Adjust phase gradually, after output
    if (done == false) {
      phase += phase_increment * rate;
      if (phase >= next) {
        
        if (index < bufFrames-1) {
          index++;
          next += bufData[index*bufChannels];
          //std::cout << "index: " << index << " next:" << next << " frame:" << x << "\n";
        } else {
          done = true;
          phase = next;
          index = bufFrames - 1;
          //printf("PlayBufS: Played to end. index:%i next:%f phase:%f\n", index, next, phase);
        }
      }
      
    }

  }
  
  if (done)
    DoneAction((int)IN0(4), unit);

  unit->mDone = done;
  unit->m_index = index;
  unit->m_phase = phase;
  unit->m_next = next;
  unit->m_prevtrig = trig;
}


void RecordBufS_Ctor(RecordBufS *unit)
{
  
  unit->m_fbufnum = -1e9f;
  unit->mIn = 0;
  
  unit->m_writepos = -1;
  unit->m_previnval = 0;

  unit->m_phase = 0;
  unit->m_lastphase = 0;

  SETCALC(RecordBuf_next);

  //printf("RecordBufS_Ctor\n");

  ClearUnitOutputs(unit, 1);
}
  
void RecordBufS_Dtor(RecordBufS *unit)
{
  TAKEDOWN_IN
}

void RecordBuf_next(RecordBufS *unit, int inNumSamples)
{

  GET_BUF
  CHECK_BUF
  SETUP_IN_ST(3)

  float run     = ZIN0(1);
  float inval     = *++(in[0]);
  int32 writepos = unit->m_writepos;
  double phase = unit->m_phase;
 
//  table0[0] = *++(in[0]);


//  if (writepos > bufSamples) {   
//    writepos = 0;
//    printf("RecordBufS: writepos: %i; run: %f; bufSamples: %i, fbufnum: %f, inval: %f\n", writepos, run, bufSamples, fbufnum, inval);
//  }

  //if (inval > 0.f && unit->m_previnval <= 0.f) {
  

  if (run > 0.f) {

    if (writepos < 0) {
      float* table0 = bufData;
      writepos = 0;
      if (bufSamples == 0) {
        unit->mDone = true;
      } else {
        table0[1] = inval;
        for (uint32 i = 1, j = 2; j < bufChannels; ++i, ++j) {
          table0[j] = *++(in[i]);
        }
      }
    } else {
      // Write time into last note
      float* table0 = bufData + writepos;
      table0[0] = phase - unit->m_lastphase + BUFDUR; // Write next phase tentatively in case the synth is freed
      if (abs(inval - unit->m_previnval) > 0.f) {
        table0[0] = phase - unit->m_lastphase; // Write current phase

        //printf("RecordBufS: wrote time %f to writepos %i. ", table0[0], writepos);

        // Shift to next note and write values, time will be written at next invalger
        writepos += bufChannels;
        if (writepos >= bufSamples) {
          // ... or quit if we're full
          unit->mDone = true;
        } else {
          table0 = bufData + writepos;
          table0[1] = inval;
          for (uint32 i = 1, j = 2; j < bufChannels; ++i, ++j) {
            table0[j] = *++(in[i]);
          }

  //        printf("wrote values ");
  //        for (uint32 i = 1; i < bufChannels; i++) {
  //          printf("%f ", table0[i]);
  //        }
  //        printf("at time %f to writepos %i\n", phase, writepos);

          unit->m_lastphase = phase; 
        }
      }
    }

    phase += BUFDUR;
    
    OUT(0)[0] = writepos;
  
    unit->m_writepos = writepos;
    unit->m_previnval = inval;
    unit->m_phase = phase;

  }

  if (unit->mDone)
    DoneAction(IN0(2), unit);
}

void PlayBufSIndex_Ctor(PlayBufSIndex* unit)
{
  SETCALC(PlayBufSIndex_next_k);

  // no need for checkBufferST, only uses first channel, only outputs one channel

  unit->m_fbufnum = -1e9f;
  unit->m_val = 0;

  PlayBufSIndex_next_k(unit, 1);
}

void PlayBufSIndex_next_k(PlayBufSIndex *unit, int inNumSamples)
{
  float trig = ZIN0(1);
  float startPos = ZIN0(2);
  float controlDurTrunc = ZIN0(3);

  GET_BUF_SHARED

  // no need for checkBufferST, only uses first channel, only outputs one channel

  double val = unit->m_val;

  double preval = val;

  if (trig > 0.f && unit->m_prevtrig <= 0.f) {
    val = 0, preval = 0;
    float controlDur = unit->mWorld->mFullRate.mBufDuration; 
    for (uint32 index = 0; index < bufFrames; index ++) {
      float length = bufData[index*bufChannels];

      if (controlDurTrunc > 0.f) {
        //printf("PlayBufSIndex: controlDurTruncing %f by %f to %f\n", length, fmod(length, controlDur), length-fmod(length,controlDur)); 
        length -= fmod(length, controlDur);
      }
      //printf("PlayBufSIndex: controlDurTrunc:%f\n", controlDurTrunc); 
      val += length;
//      printf("PlayBufSIndex: val:%f index:%i bufFrames:%i bufChannels:%i\n", val, index, bufFrames, bufChannels);
      if (val > startPos) {
//        printf("PlayBufSIndex: break at preval %f\n", preval);
        break;
      }
      preval = val;
    }
    val = preval;
  }

  OUT(0)[0] = val;

  unit->m_val = val;
  unit->m_prevtrig = trig;
}

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

void DetectEndS_Ctor(DetectEndS* unit)
{
  SETCALC(DetectEndS_next_k);
  unit->m_fbufnum = -1e9f;
  DetectEndS_next_k(unit, 1);
}

void DetectEndS_next_k(DetectEndS *unit, int inNumSamples)
{
  GET_BUF_SHARED
  uint32 out = 0;
  uint32 j = 0;
  for (uint32 i = 0; i < bufSamples; i = i + bufChannels) {
    if (bufData[i] == 0) {
      out = j;
      break;
    }
    j++;
  }
  OUT(0)[0] = out;
}

PluginLoad(PlayBufS)
{
    ft = inTable;
    DefineSimpleUnit(PlayBufSIndex);
    DefineSimpleUnit(BufSet);
    DefineSimpleUnit(PlayBufS);
    DefineDtorUnit(RecordBufS);
    DefineSimpleUnit(DetectEndS);
}

