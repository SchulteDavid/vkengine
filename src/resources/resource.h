#ifndef RESOURCE_H
#define RESOURCE_H

#include "resources/resourcelocation.h"

class Resource {

public:
  Resource(std::string type) : location(type, "__undefined__", "") {
    
  }
  virtual ~Resource() {}

  void setLocation(const ResourceLocation & location);
  const ResourceLocation & getLocation();
  
protected:

private:

  ResourceLocation location;
  

};

#endif // RESOURCE_H
