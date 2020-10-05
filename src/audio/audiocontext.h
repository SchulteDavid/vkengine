#ifndef AUDIO_CONTEXT_H
#define AUDIO_CONTEXT_H

#include "util/transform.h"

namespace audio {

  struct AudioContextData;
  
  class AudioContext {

  public:
    AudioContext();
    virtual ~AudioContext();

    void setListenerTransform(Transform<double> trans);

  private:
    AudioContextData * data;

  };

};

#endif
