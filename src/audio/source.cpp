#include "audio/source.h"

#include <AL/al.h>

using namespace audio;

Source::Source() {

  alGenSources(1, &sourceId);
  
}

Source::~Source() {
  alDeleteSources(1, &sourceId);
}

void Source::play(std::shared_ptr<Sound> sound) {
  alSourceQueueBuffers(sourceId, 1, &sound->buffer);
  ALenum sourceState;
  alGetSourcei(sourceId, AL_SOURCE_STATE, &sourceState);
  if (sourceState == AL_STOPPED || sourceState == AL_INITIAL)
    alSourcePlay(sourceId);
}

void Source::setTransform(Transform<double> transform) {
  alSource3f(sourceId, AL_POSITION, transform.position[0], transform.position[1], transform.position[2]);
}
