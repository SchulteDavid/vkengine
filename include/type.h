#ifndef _CONFIG_TYPE_H
#define _CONFIG_TYPE_H

#include <string>
#include <cstdint>

class CompoundNode;

enum class ValueType : uint64_t {
	
	E_FILE_END = 0x0,

	E_VOID    = 0x0000000064696f76, 		//'void'
	E_BOOLEAN = 0x000000006c6f6f62, 		//'bool'
	E_INT32   = 0x0000003233746e69,		//'int32'
	E_INT64   = 0x0000003436746e69,		//'int64'
	E_FLOAT32 = 0x00323374616f6c66,		//'float32'
	E_FLOAT64 = 0x00343674616f6c66,		//'float64'
	E_STRING  = 0x0000676e69727473,		//'string'
	E_COMP    = 0x00000000706d6f63,		//'comp'
	E_COMPEND = 0x00646e65706d6f63,		//'compend'
	E_STRUCT  = 0x0000746375727473,		//'struct'
	
};

typedef struct structure_element_t {
	
	std::string name;
	ValueType type;
	
} STRUCT_ELEM;

const uint64_t BOOLEAN_NAME = (const uint64_t) typeid(bool).name();
const uint64_t INT32_NAME   = (const uint64_t) typeid(int).name();
const uint64_t INT64_NAME   = (const uint64_t) typeid(long).name();
const uint64_t FLOAT32_NAME = (const uint64_t) typeid(float).name();
const uint64_t FLOAT64_NAME = (const uint64_t) typeid(double).name();
const uint64_t STRING_NAME  = (const uint64_t) typeid(const char *).name();

template <typename T> T fromString(std::string string) {
	
	uint64_t typeName = (uint64_t) typeid(T).name();
	
	if (typeName == BOOLEAN_NAME) {
		return !string.compare("true");
	} else if (typeName == INT32_NAME) {
		return stoi(string);
	} else if (typeName == INT64_NAME) {
		return stol(string);
	} else if (typeName == FLOAT32_NAME) {
		return stof(string);
	} else if (typeName == FLOAT64_NAME) {
		return stod(string);
	}
}

template <typename T> ValueType getTypeFromTemplate() {
	
	uint64_t typeName = (uint64_t) typeid(T).name();
	
	if (typeName == BOOLEAN_NAME) {
		return ValueType::E_BOOLEAN;
	} else if (typeName == INT32_NAME) {
		return ValueType::E_INT32;
	} else if (typeName == INT64_NAME) {
		return ValueType::E_INT64;
	} else if (typeName == FLOAT32_NAME) {
		return ValueType::E_FLOAT32;
	} else if (typeName == FLOAT64_NAME) {
		return ValueType::E_FLOAT64;
	} else if (typeName == STRING_NAME) {
		return ValueType::E_STRING;
	} else if (typeName == (uint64_t) typeid(CompoundNode *).name()) {
		return ValueType::E_COMP;
	}
		return ValueType::E_STRUCT;
}

std::string typeToString(ValueType type);

#endif
