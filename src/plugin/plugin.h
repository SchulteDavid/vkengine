#ifndef PLUGIN_H
#define PLUGIN_H

#include "render/window.h"

class Plugin {
public:
  Plugin();
  virtual ~Plugin();
  

  virtual void preload();
  virtual void onInit(Window * window);
  
protected:
  
private:
  
  

};

#endif // PLUGIN_H
