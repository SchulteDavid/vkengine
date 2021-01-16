#ifndef PLAYERCONTROLER_H
#define PLAYERCONTROLER_H

#include <memory>

#include "inputhandler.h"
#include "render/camera.h"
#include "render/util/vkutil.h"

#include "audio/audiocontext.h"
#include "node/node.h"

class PlayerControler : public InputHandler {
public:
  PlayerControler(std::shared_ptr<Camera> camera, vkutil::VulkanState & state, std::shared_ptr<audio::AudioContext> audioContext, std::shared_ptr<strc::Node> node);
  virtual ~PlayerControler();

  void onMouseMotion(double xpos, double ypos, double dx, double dy);
  void onKeyboard(int key, int scancode, int action, int mods);
  void onMouseButton(int button, int action, int mods);
  void onScroll(double dx, double dy);

  Viewport * viewport;
  std::shared_ptr<World> world;

protected:

private:

  vkutil::VulkanState & state;

  std::shared_ptr<Camera> camera;
  bool hasCursor;

  double phi;
  double theta;
  double radius;

  std::shared_ptr<audio::AudioContext> audioContext;

  std::shared_ptr<strc::Node> throwNode;

  void updateCamera();

};

#endif // PLAYERCONTROLER_H
