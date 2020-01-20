#ifndef _CONFIGLOADER_H
#define _CONFIGLOADER_H

#include "node.h"

class ConfigLoader {

	public:
		static CompoundNode * loadFileTree(std::string filename);

};


#endif
