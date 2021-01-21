#include "node/audiosourcenode.h"

using namespace strc;
using namespace audio;

AudioSourceNode::AudioSourceNode(std::string name, Transform<double> transform) : Node(name, transform) {
  this->source = std::make_shared<Source>();
}

void AudioSourceNode::playSound(std::shared_ptr<audio::Sound> sound) {
  source->play(sound);
}

void AudioSourceNode::onTransformUpdate() {
  source->setTransform(getGlobalTransform());
}

std::shared_ptr<Node> AudioSourceNode::duplicate(std::string name) {
  return std::make_shared<AudioSourceNode>(name, transform);
}

std::shared_ptr<NodeUploader> strc::loadAudioSourceNode(std::shared_ptr<config::NodeCompound> root, const NodeLoader::LoadingContext &context, const std::string nodeName) {
  return std::make_shared<NodeUploader>(std::make_shared<AudioSourceNode>(nodeName, context.transform));
}

void AudioSourceNode::saveNode(std::shared_ptr<config::NodeCompound> comp) {

}
