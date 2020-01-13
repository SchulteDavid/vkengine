#ifndef _PLY_H
#define _PLY_H

#include <list>
#include <string>
#include <vector>

typedef enum ply_var_type_t {
	
	PLY_FLOAT,
	PLY_UINT,
	PLY_UCHAR,
	
	PLY_NONE = -1
	
} PlyVarType;

typedef struct ply_property_t {
	
	std::string name;
	PlyVarType type;
	PlyVarType countType;
	bool isList;
	
} PlyProperty;

class PlyElement {
	
	public:
		PlyElement(std::string name, int count);
		
		void addProperty(std::string name, std::string type);
		void addProperty(std::string countType, std::string valueType, std::string name);
		
		int getElementCount();
		int getPropertyCount();
		
		bool hasUvCoords();
		bool hasColorData();
		
		PlyProperty getProperty(int index);
		
	private:
		int elementCount;
		std::string name;
		
		std::vector<PlyProperty> properties;
	
};

class PlyFile {
	
	public:
		PlyFile(std::string filename);
		
		void addElementDef(PlyElement * element);
		
		float * getVertexData(int * count);
		int * getIndexData(int * count);
		
	private:
		std::vector<PlyElement*> elements;
		
		int vertexCount;
		int indexCount;
		float * vertices;
		int * indecies;
	
};


#endif
