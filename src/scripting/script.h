#ifndef SCRIPT_H
#define SCRIPT_H

#include "render/window.h"

#include "resources/resource.h"

namespace scripting {

  
  
  class Script : public Resource {
  public:
    Script();
    virtual ~Script();
  

    virtual void preload();
    virtual void onInit(Window * window);
  
  protected:
  
  private:
  
  

  };

}

#endif
