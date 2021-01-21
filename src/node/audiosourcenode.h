#ifndef AUDIO_SOURCE_NODE_H
#define AUDIO_SOURCE_NODE_H

#include "node.h"
#include "audio/source.h"
#include "node/nodeloader.h"

namespace strc {

  class AudioSourceNode : public Node {

  public:
    AudioSourceNode(std::string name, Transform<double> transform);

    void playSound(std::shared_ptr<audio::Sound> sound);

    void saveNode(std::shared_ptr<config::NodeCompound> comp) override;
    
  protected:
    void onTransformUpdate() override;

    std::shared_ptr<Node> duplicate(std::string name) override;
    
  private:

    std::shared_ptr<audio::Source> source;
    
  };

  std::shared_ptr<NodeUploader> loadAudioSourceNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext & context, const std::string nodeName);

};

#endif
