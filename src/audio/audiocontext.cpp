#include "audio/audiocontext.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

#include <stdlib.h>

#include <iostream>
#include <list>
#include <string>
#include <memory>
#include <vector>

#include "util/debug/trace_exception.h"

#include "audio/sound.h"

namespace audio {

  struct AudioContextData {

    ALCdevice * device;
    ALCcontext * context;
  
  };


  struct WavFileHeader {
    uint32_t magic;
    uint32_t size;
    uint32_t riffType;
  };
  
  struct RiffChuckHeader {
    uint32_t type;
    uint32_t size;
  };

  struct WavFormatInfo {
    uint16_t compressionCode;
    uint16_t chanelCount;
    uint32_t sampleRate;
    uint32_t bytesPerSecond;
    uint16_t blockAlign;
    uint16_t significantBits;
  };

  enum RiffChunkTypes {
		       CHUNK_DATA = 0x61746164,
		       CHUNK_FORMAT = 0x20746d66
  };
  
  std::vector<uint8_t> loadWavData(const char * fname, SoundFormatInfo * outInfo) {

    FILE * file = fopen(fname, "rb");
    if (!file)
      throw dbg::trace_exception(std::string("Cannot open audio-file: ").append(fname));

    WavFileHeader wavHeader;
    fread(&wavHeader, sizeof(WavFileHeader), 1, file);

    RiffChuckHeader chunkHeader;
    std::vector<uint8_t> chunkData;

    WavFormatInfo formatInfo = {};

    while (fread(&chunkHeader, sizeof(RiffChuckHeader), 1, file)) {

      std::cout << "Chunk header at offset " << std::hex << ftell(file) << std::dec << std::endl;
      
      switch (chunkHeader.type) {

      case CHUNK_DATA:
	chunkData.resize(chunkHeader.size);
	fread(chunkData.data(), sizeof(uint8_t), chunkHeader.size, file);
	outInfo->dataSize = chunkHeader.size;
	break;

      case CHUNK_FORMAT:
	std::cout << "Reading format info at offset " << std::hex << ftell(file) << std::dec << std::endl;
	fread(&formatInfo, sizeof(WavFormatInfo), 1, file);
	break;
	
      default:
	fseek(file, chunkHeader.size, SEEK_CUR);
	
      }
      /// Align to 16 bits
      while (ftell(file) % 2)
	fseek(file, 1, SEEK_CUR);
    }

    if (formatInfo.significantBits == 16 && formatInfo.chanelCount == 1) {
      outInfo->format = AL_FORMAT_MONO16;
    } else if (formatInfo.significantBits == 8 && formatInfo.chanelCount == 1) {
      outInfo->format = AL_FORMAT_MONO8;
    } else if (formatInfo.significantBits == 16 && formatInfo.chanelCount == 2) {
      outInfo->format = AL_FORMAT_STEREO16;
    } else if (formatInfo.significantBits == 8 && formatInfo.chanelCount == 2) {
      outInfo->format = AL_FORMAT_STEREO8;
    } else {
      std::cout << "channels " << formatInfo.chanelCount << " bits: " << formatInfo.significantBits << std::endl;
      throw dbg::trace_exception("Unknown channel configuration");
    }

    outInfo->sampleRate = formatInfo.sampleRate;

    return chunkData;
    
  }

  std::list<std::string> getDeviceList() {

    std::list<std::string> devices;

    const char * data = alcGetString(NULL, ALC_DEVICE_SPECIFIER);

    do {

      std::string name(data);
      std::cout << name << std::endl;

      data += name.size();

      devices.insert(devices.end(), name);
      
    } while(*(data+1));

    return devices;
    
  }

}

using namespace audio;

AudioContext::AudioContext() {

  this->data = new AudioContextData();

  std::list<std::string> devices = getDeviceList();
  
  data->device = alcOpenDevice(devices.front().c_str());

  if (!data->device)
    throw dbg::trace_exception("Unable to open audio device");
  
  data->context = alcCreateContext(data->device, NULL);
  alcMakeContextCurrent(data->context);

}

AudioContext::~AudioContext() {

  alcDestroyContext(data->context);
  alcCloseDevice(data->device);
  
  delete data;

}

void AudioContext::setListenerTransform(Transform<double> trans) {

  alListener3f(AL_POSITION, trans.position[0], trans.position[1], trans.position[2]);

  Math::Matrix<3, 3> rotation = trans.rotation.toRotationMatrix();
  Math::Vector<3> front = rotation * Math::Vector<3>({0, 1, 0});
  Math::Vector<3> up = rotation * Math::Vector<3>({0,0,1});

  float data[6] = {(float) front[0], (float)front[1], (float)front[2], (float)up[0], (float)up[1], (float)up[2]};

  alListenerfv(AL_ORIENTATION, data);
  
}

namespace audio {
  class SoundUploader : public ResourceUploader<Sound> {

  public:
    SoundUploader(std::shared_ptr<audio::Sound> s) {
      sound = s;
    }

    std::shared_ptr<Sound> uploadResource(vkutil::VulkanState & state) override {
      return sound;
    }

    bool uploadReady() {
      return true;
    }

  private:
    std::shared_ptr<Sound> sound;
    
  };
}

std::shared_ptr<ResourceUploader<Sound>> SoundLoader::loadResource(std::string fname) {

  SoundFormatInfo formatInfo;
  std::vector<uint8_t> data = loadWavData(fname.c_str(), &formatInfo);

  std::shared_ptr<Sound> sound = std::make_shared<Sound>(data, formatInfo);

  return std::make_shared<SoundUploader>(sound);
  
}
