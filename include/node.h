#ifndef _CONFIG_NODE_H
#define _CONFIG_NODE_H

#include <string>
#include <vector>
#include <iostream>
#include <typeinfo>
#include <string.h>
#include <map>
#include <list>

#include "type.h"

template <typename T> class Node {

	public:
		Node (std::string name) {
			this->name = name;
			this->valType = getTypeFromTemplate<T>();
		}
		Node (std::string name, T val) {
			this->name = name;
			this->value = val;
			this->valType = getTypeFromTemplate<T>();

		}
		Node () {

		}
		virtual ~Node(){

		}

		virtual T getValue() {
			return value;
		}
		operator T() {return getValue();}

		operator std::string() {return "NODE\n";}

		virtual void setValue(T val) {
			this->value = val;
		}

		virtual bool hasChildren() {return false;}

		void setName(std::string name) {
			this->name = name;
		}

		std::string getName() {
			return name;
		}

		virtual bool isArray() {return false;}
		virtual ValueType getType() {
			return valType;
		}

		virtual void writeToBinaryFile(FILE * file) {

			if (valType != ValueType::E_STRING)
				fwrite(&value, sizeof(T), 1, file);
			else {
				const char ** test = (const char**) &value;
				std::string data(*test);
				short length = data.size();
				fwrite(&length, sizeof(short), 1, file);
				fwrite(data.c_str(), sizeof(char), length, file);

			}

		}

	protected:

		std::string name;
		ValueType valType;
		T value;

};
template <typename T> class ArrayNode;
class CompoundNode : public Node<Node<void*>> {


	public:

		CompoundNode ();

		CompoundNode (std::string name);

		virtual ~CompoundNode() {
			for (auto it = children.begin(); it != children.end(); ++it) {
				delete it->second;
			}
		}

		virtual int getChildCount();
		virtual Node<void*> * getChildNode(std::string index);

		void addChildNode(Node<void*> * node);

		std::map<std::string, Node<void*> *> getChildMap() {
			return this->children;
		}

		virtual ValueType getType();

		template <typename T> Node<T> * getNode(std::string name) {
			return (Node<T> *) this->children[name];
		}
		template <typename T> ArrayNode<T> * getArray(std::string name) {
			return (ArrayNode<T> *) this->children[name];
		}
		template <typename T> T get(std::string name) {
			std::cout << "Getting Value " << name << " from " << this->name << std::endl;
			return ((Node<T> *) this->children[name])->getValue();
		}
		CompoundNode * getCompound(std::string name) {
			return (CompoundNode *) this->children[name];
		}

		std::list<std::string> getChildList();

		virtual void writeToBinaryFile(FILE * file) override;


	protected:
		std::map<std::string, Node<void*> *> children;

};

class StructureNode : public CompoundNode {

	public:
		StructureNode (std::list<STRUCT_ELEM> def);
		StructureNode (std::list<STRUCT_ELEM> def, std::string name);

		std::list<STRUCT_ELEM> getDef();

		void writeToBinaryFile(FILE * file);

		template <typename T> void set(std::string name, T value) {
			((Node<T>*)this->children[name])->setValue(value);
		}

		ValueType getType() override {
			return ValueType::E_STRUCT;
		}

		static void writeDefToFile(FILE * file, std::list<STRUCT_ELEM> def);
		static void writeDefToFile(FILE * file, std::list<STRUCT_ELEM> def, bool bin);

	protected:
		std::list<STRUCT_ELEM> definition;

};

template <typename T> class ArrayNode : public Node<std::vector<T>> {

	public:

		ArrayNode (std::string name) : Node<std::vector<T>> (name) {
			this->size = 0;
			this->valType = getTypeFromTemplate<T>();
		}

		ArrayNode (std::string name, std::vector<T> val) : Node<std::vector<T>> (name, val) {
			this->size = val.size();
			this->valType = getTypeFromTemplate<T>();
		}

		ArrayNode () {
			this->size = 0;
			this->valType = getTypeFromTemplate<T>();
		}

		ArrayNode (std::string name, std::list<std::string> stringValues) : Node<std::vector<T>> (name) {

			this->value.reserve(stringValues.size());

			for (std::list<std::string>::iterator it = stringValues.begin(); it != stringValues.end(); ++it) {

				this->value.push_back(fromString<T>(*it));

			}

			this->size = this->value.size();
			this->valType = getTypeFromTemplate<T>();

		}

		virtual void addValue(T val) {
			this->value.push_back(val);
			this->size++;
		}

		virtual void writeToBinaryFile(FILE * file) override {
			if (this->valType == ValueType::E_COMP) {
				for (int i = 0; i < this->size; ++i) {

					CompoundNode ** tmp;
					tmp = (CompoundNode**) &this->value[i];
					(*tmp)->writeToBinaryFile(file);
					//void * loc = this->value.data();
					//this->value[i]->writeToBinaryFile(file);
				}
				return;
			}
			else if (this->valType == ValueType::E_STRUCT) {
				StructureNode ** tmp = (StructureNode **) &this->value[0];
				StructureNode::writeDefToFile(file, (*tmp)->getDef());

				for (int i = 0; i < this->size; ++i) {

					tmp = (StructureNode**) &this->value[i];
					(*tmp)->writeToBinaryFile(file);
				}
				return;

			}
			for (int i = 0; i < this->size; ++i) {

				Node<T> * tmp = new Node<T>("", this->value[i]);
				tmp->writeToBinaryFile(file);

			}

		}

		virtual bool isArray() override {return true;}
		virtual int getArraySize() {
			return this->value.size();
		}

	protected:
		int size;

};

CompoundNode * readBinaryCompound(FILE * file, std::string compoundName);

#endif
