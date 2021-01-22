#ifndef SCRIPT_H
#define SCRIPT_H

#include <any>

#include "render/window.h"

#include "resources/resource.h"

class Script : public Resource {

public:
  Script();
  virtual ~Script();
  

  /// Function called before the object is finally loaded
  virtual void preload() = 0;

  /// Called on 'uploading' the object to the resource-manager.
  virtual void onInit(Window * window) = 0;

  /// This checks if a script has an attribute by the name 'name'.
  virtual bool hasObject(std::string name) = 0;

  /// Gets an object from the script
  virtual std::any getObject(std::string name) = 0;
  
protected:
  
private:
  
  

};

#endif
