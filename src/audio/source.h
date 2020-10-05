#ifndef AUDIO_SOURCE_H
#define AUDIO_SOURCE_H

#include <stdint.h>

#include "audio/sound.h"
#include "util/transform.h"

namespace audio {

  class Source {

  public:
    Source();
    virtual ~Source();

    void play(std::shared_ptr<Sound> sound);

    void setTransform(Transform<double> transform);


  private:
    uint32_t sourceId;

  };
  
}

#endif
