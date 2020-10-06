#ifndef PLAYERCONTROLER_H
#define PLAYERCONTROLER_H

#include <memory>

#include "inputhandler.h"
#include "render/camera.h"
#include "render/util/vkutil.h"

#include "audio/audiocontext.h"

class PlayerControler : public InputHandler {
public:
  PlayerControler(Camera * camera, const vkutil::VulkanState & state, std::shared_ptr<audio::AudioContext>);
  virtual ~PlayerControler();

  void onMouseMotion(double xpos, double ypos, double dx, double dy);
  void onKeyboard(int key, int scancode, int action, int mods);
  void onMouseButton(int button, int action, int mods);
  void onScroll(double dx, double dy);

protected:

private:

  const vkutil::VulkanState & state;

  Camera * camera;
  bool hasCursor;

  double phi;
  double theta;
  double radius;

  std::shared_ptr<audio::AudioContext> audioContext;

  void updateCamera();

};

#endif // PLAYERCONTROLER_H
