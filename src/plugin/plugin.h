#ifndef PLUGIN_H
#define PLUGIN_H

#include "render/window.h"

#include "resources/resource.h"

class Plugin : public Resource {
public:
  Plugin();
  virtual ~Plugin();
  

  virtual void preload();
  virtual void onInit(Window * window);
  
protected:
  
private:
  
  

};

#endif // PLUGIN_H
