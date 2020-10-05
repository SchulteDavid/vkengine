#ifndef SOUND_H
#define SOUND_H

#include <vector>
#include <stdint.h>

#include "resources/resource.h"
#include "resources/resourceloader.h"

namespace audio {

  class Source;
  
  struct SoundFormatInfo {
    uint32_t format;
    uint32_t dataSize;
    uint32_t sampleRate;
  };
  
  class Sound : public Resource {
  public:
    Sound(std::vector<uint8_t> data, SoundFormatInfo format);
    virtual ~Sound();

    friend Source;
    
  protected:
    uint32_t buffer;
    
  private:
    std::vector<uint8_t> data;
    const SoundFormatInfo format;
  };

  class SoundLoader : public ResourceLoader<Sound> {

  public:
    std::shared_ptr<ResourceUploader<Sound>> loadResource(std::string fname);
    
  };

}

#endif
