#include "audio/sound.h"

#include <AL/al.h>

using namespace audio;

Sound::Sound(std::vector<uint8_t> data, SoundFormatInfo formatInfo) : Resource("Sound"), format(formatInfo) {

  this->data = data;

  alGenBuffers(1, &buffer);
  alBufferData(buffer, format.format, data.data(), format.dataSize, format.sampleRate);
  
}

Sound::~Sound() {
  alDeleteBuffers(1, &buffer);
}
