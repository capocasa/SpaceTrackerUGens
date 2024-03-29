/*
    SpaceTrackerUGens for SuperCollider 
    Copyright (c) 2014-2017 Carlo Capocasa. All rights reserved.
    https://capocasa.net

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "SC_PlugIn.h"
#include "stdio.h"

static InterfaceTable *ft;

// For RecordSpaceTracker
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
      Print("RecordSpaceTracker channel mismatch: numInputs %i, yet buffer has %i channels. buffer needs one more channel than numInputs to store time\n", numInputs, bufChannels); \
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

struct PlaySpaceTracker : public Unit
{
  double m_phase;
  float m_fbufnum;
  uint32 m_index;
  double m_next;
  SndBuf *m_buf;
  float m_prevtrig;
  float m_prevbufnum;
};

struct RecordSpaceTracker : public Unit
{
  int32 m_writepos;
  float **mIn;
  float m_fbufnum;
  SndBuf *m_buf;
  float m_previnval;
  double m_phase;
  double m_lastphase;
};

struct IndexSpaceTracker : public Unit
{
  float m_fbufnum;
  SndBuf *m_buf;
  float m_prevtrig;
  double m_val;
};


struct SpaceTrackerFrames: public Unit
{
  float m_fbufnum;
  SndBuf *m_buf;
  float prevtrig;
  uint32 offset;
  uint32 count;
  float pre;
  float post;
};

static void PlaySpaceTracker_next(PlaySpaceTracker *unit, int inNumSamples);
static void PlaySpaceTracker_Ctor(PlaySpaceTracker* unit);

static void RecordSpaceTracker_next(RecordSpaceTracker *unit, int inNumSamples);
static void RecordSpaceTracker_Ctor(RecordSpaceTracker *unit);

static void IndexSpaceTracker_next_k(IndexSpaceTracker *unit, int inNumSamples);
static void IndexSpaceTracker_Ctor(IndexSpaceTracker* unit);

static void SpaceTrackerFrames_next_k(SpaceTrackerFrames *unit, int inNumSamples);
static void SpaceTrackerFrames_Ctor(SpaceTrackerFrames* unit);

void PlaySpaceTracker_Ctor(PlaySpaceTracker* unit)
{
  SETCALC(PlaySpaceTracker_next);

  unit->m_fbufnum = -1e9f;
  unit->m_prevbufnum = -1e9f;
  unit->m_phase = 0; 
  unit->m_next = 0;
  unit->m_index = 0;
  unit->m_prevtrig = 0;

  //PlaySpaceTracker_next(unit, 1);

  // std::cout.precision(17);

}

void PlaySpaceTracker_next(PlaySpaceTracker *unit, int inNumSamples)
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

  bool loop = IN0(4);

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
        done = true;
      }
      //printf("PlaySpaceTracker: initialized. rate:%f phase:%f next: %f\n bufnum:%f prevbufnum:%f\n", rate, phase, next, bufnum, prevbufnum);
    }
    unit->m_prevbufnum = bufnum;

    // Respont to triggers instantly, before output
    
    if (trig > 0.f && unit->m_prevtrig <= 0.f) {
    
      //printf("PlaySpaceTracker: triggered. phase:%f next:%f time:%f bufnum:%f note:%f value:%f\n", phase, next, unit->m_fbufnum, frame[0], frame[1], frame[2]);
      
      //printf("PlaySpaceTracker: phase:%f next:%f \n", phase, next);
      
      phase = IN(3)[x];
      
      
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
          done = true;
        }
      } else {
        if (next > phase) {
          double prevnext;
          while (true) {
            //printf("PlaySpaceTracker: trackback index:%i next:%f phase:%f\n", index, next, phase);
            prevnext = next;
            next -= bufData[index*bufChannels];
            
            if (next <= phase) {
              next = prevnext;
              break;
            }

            index--;
          }
          //printf("PlaySpaceTracker: trackbacked. index:%i next:%f phase:%f\n", index, next, phase);
        } else {
          // if phase==next, do nothing
          while (next < phase) {
            //printf("PlaySpaceTracker: catchup index:%i next:%f phase:%f\n", index, next, phase);
            index++;
            if (index >= bufFrames) {
              done = true;
              phase = next;
              index = bufFrames - 1;
              break;
            }
            float time = bufData[index*bufChannels];
            if (time == 0) {
              done = true;
              phase = next;
              index--;
              break;
            }
            next += time;
          }
          //printf("PlaySpaceTracker: caught up. index:%i next:%f phase:%f\n", index, next, phase);
        }
      }

    }

    frame = bufData + index * bufChannels;
    
    // debug output
    //printf("out %f %f - %i %i %i %f - %f . %f %f %f\n", rate, phase, index, bufFrames, done, trig, frame[0], frame[1], frame[2], frame[3]);

    for (int i = 0, j = 1; j < bufChannels; i++, j++) {
      if (rate > 0 && done == false) {
        OUT(i)[x] = frame[j];
      } else {
        OUT(i)[x] = 0;
      }
    }
    
    // Adjust phase gradually, after output
    if (done == false) {
      phase += phase_increment * rate;
 
      if (phase < 0) {
        if (phase <= next) {

          if (index > 0) {
            index--;
            next -= bufData[index*bufChannels];
            //std::cout << "index: " << index << " next:" << next << " frame:" << x << "\n";
          } else {
            if (loop) {
              for (index = 0; index < bufFrames; index++) {
                phase += bufData[index];
              }
              next = bufData[index];
            } else {
              done = true;
              phase = next;
              index = 0;
              //printf("PlaySpaceTracker: Played to end. index:%i next:%f phase:%f\n", index, next, phase);
            }
          }
        }
      } else {
        if (phase > next) {

          if (index < bufFrames-1) {
            index++;
            next += bufData[index*bufChannels];
            //std::cout << "index: " << index << " next:" << next << " frame:" << x << "\n";
          } else {
            if (loop) {
              index = 0;
              phase = 0;
              next = bufData[0];
            } else {
              done = true;
              phase = next;
              index = bufFrames - 1;
              //printf("PlaySpaceTracker: Played to end. index:%i next:%f phase:%f\n", index, next, phase);
            }
          }
        }
      }  
    }

  }
  
  if (done)
    DoneAction((int)IN0(5), unit);

  unit->mDone = done;
  unit->m_index = index;
  unit->m_phase = phase;
  unit->m_next = next;
  unit->m_prevtrig = trig;
}


void RecordSpaceTracker_Ctor(RecordSpaceTracker *unit)
{
  
  unit->m_fbufnum = -1e9f;
  unit->mIn = 0;
  
  unit->m_writepos = 0;
  unit->m_previnval = 0;

  unit->m_phase = 0;
  unit->m_lastphase = 0;

  SETCALC(RecordSpaceTracker_next);

  ClearUnitOutputs(unit, 1);
}
  
void RecordSpaceTracker_Dtor(RecordSpaceTracker *unit)
{
  TAKEDOWN_IN
}

void RecordSpaceTracker_next(RecordSpaceTracker *unit, int inNumSamples)
{

  GET_BUF
  CHECK_BUF
  SETUP_IN_ST(3)
  
  bool audiorate = inNumSamples != 1;

  double phase_increment;
  if (audiorate) {
    phase_increment = SAMPLEDUR;
  } else {
    phase_increment = BUFDUR;
  }

  float run     = ZIN0(1);
  int32 writepos = unit->m_writepos;
  double phase = unit->m_phase;

  float previnval = unit->m_previnval;
  double lastphase = unit->m_lastphase;

  bool done = unit->mDone;

//  if (writepos > bufSamples) {   
//    writepos = 0;
//    printf("RecordSpaceTracker: writepos: %i; run: %f; bufSamples: %i, fbufnum: %f, inval: %f\n", writepos, run, bufSamples, fbufnum, inval);
//  }

  if (bufFrames == 0) {
    done = true;
  }

  if (run > 0.f && done==false) {

    for (int x = 0; x < inNumSamples; x++) {
      
      float inval;
      float* table0;
      float time;
        
      inval = *++(in[0]);
      table0 = bufData + writepos;

      // Write time into last note
      time = phase - lastphase;
      table0[0] = time + phase_increment; // Write next phase tentatively in case the synth is freed
      if (abs(inval - previnval) > 0.f) {
        if (phase > 0) {
          table0[0] = time; // Write current phase
          
          //printf("RecordSpaceTracker: wrote time %f with note %f value %f on writepos %i in frame %i. \n", time, table0[1], table0[2], writepos, x);

          // Shift to next note and write values, time will be written at next
          writepos += bufChannels;
        }
        if (writepos >= bufSamples) {
          // ... or quit if we're full
          done = true;
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

        }
        lastphase = phase; 
      } else {
        // advance unused pointers
        for (uint32 i = 1, j = 2; j < bufChannels; ++i, ++j) {
          *++(in[i]);
        }
      }

      previnval = inval;
      phase += phase_increment;

      OUT(0)[0] = writepos;

    }

    unit->m_lastphase = lastphase;
    unit->m_writepos = writepos;
    unit->m_previnval = previnval;
    unit->m_phase = phase;
    unit->mDone = done;
  }

  if (done)
    DoneAction(IN0(2), unit);
}

void IndexSpaceTracker_Ctor(IndexSpaceTracker* unit)
{
  SETCALC(IndexSpaceTracker_next_k);

  // no need for checkBufferST, only uses first channel, only outputs one channel

  unit->m_fbufnum = -1e9f;
  unit->m_val = 0;

  IndexSpaceTracker_next_k(unit, 1);
}

void IndexSpaceTracker_next_k(IndexSpaceTracker *unit, int inNumSamples)
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
        //printf("IndexSpaceTracker: controlDurTruncing %f by %f to %f\n", length, fmod(length, controlDur), length-fmod(length,controlDur)); 
        length -= fmod(length, controlDur);
      }
      //printf("IndexSpaceTracker: controlDurTrunc:%f\n", controlDurTrunc); 
      val += length;
//      printf("IndexSpaceTracker: val:%f index:%i bufFrames:%i bufChannels:%i\n", val, index, bufFrames, bufChannels);
      if (val > startPos) {
//        printf("IndexSpaceTracker: break at preval %f\n", preval);
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

void SpaceTrackerFrames_Ctor(SpaceTrackerFrames* unit)
{
  SETCALC(SpaceTrackerFrames_next_k);
  unit->m_fbufnum = -1e9f;
  unit->prevtrig = 0;
  unit->count = 0;
  unit->offset = 0;
  unit->pre = 0;
  unit->post = 0;
  SpaceTrackerFrames_next_k(unit, 1);
}

void SpaceTrackerFrames_next_k(SpaceTrackerFrames *unit, int inNumSamples)
{
  GET_BUF_SHARED
  uint32 count = unit->count;
  uint32 offset = unit->offset;
  float pre = unit->pre;
  float post = unit->post;
  float trig = IN0(1);
  float start = IN0(2);
  float end = IN0(3);
  if (end > 0) {
    end += start;
  }
  float time = 0;
  float length = 0;
  float lastlength = 0;
  float lasttime = 0;
  float nexttime = 0;

  bool preassigned = false;

  if (trig > 0.f && unit->prevtrig <= 0.f) {
    for (uint32 i = 0; i < bufSamples; i += bufChannels) {
      lastlength = length;
      length = bufData[i];
      nexttime = time + length;
//printf("3 length:%f lastlength:%f bufChannels:%i end:%f time:%f\n", length, lastlength, bufChannels, end, time);
      if (length == 0) {
        break;
      }
//printf("1 time:%f start:%f\n",time,start);
      if (start == 0) {
        count++;
      } else if (nexttime > start) {
        count++;
      } else {
        offset++;
      }

      if (start > 0 && nexttime >= start) {
        if (preassigned == false) {
          pre = nexttime - start;
//printf("8 start:%f time:%f nexttime:%f pre:%f \n", start, time, nexttime, pre);
          preassigned = true;
        }
      }

      if (end > 0 && nexttime > end) {
        post = end - time;
//printf("2 end:%f time:%f nexttime:%f post:%f \n", end, time, nexttime, post);
        
        if (time < start) { // corner case: within last note
          post = pre = end - start;
        }
        break;
      }
      if (nexttime == end) {
        break;
      }
//printf("6 offset:%i count:%i\n",offset,count);
      lasttime = time;      
      time = nexttime; 
    }
  }
 
  if (count > 0 || bufFrames == 0) {
    DoneAction((int)IN0(4), unit);
  }

  OUT0(0) = count;
  OUT0(1) = offset;
  OUT0(2) = pre;
  OUT0(3) = post;

  unit->prevtrig = trig;
  unit->count = count;
  unit->offset = offset;
  unit->pre = pre;
  unit->post = post;
}

PluginLoad(SpaceTracker)
{
    ft = inTable;
    DefineSimpleUnit(IndexSpaceTracker);
    DefineSimpleUnit(PlaySpaceTracker);
    DefineDtorUnit(RecordSpaceTracker);
    DefineSimpleUnit(SpaceTrackerFrames);
}

