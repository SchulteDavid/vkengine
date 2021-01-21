#ifndef RESOURCELOCATION_H
#define RESOURCELOCATION_H

#include <string>
#include <memory>

struct ResourceLocation {


  ResourceLocation(std::string type, std::string filename) : type(type), filename(filename), name("") {


  }

  ResourceLocation(std::string type, std::string filename, std::string name) : type(type), filename(filename), name(name) {


  }

  /*ResourceLocation() : type("UNKNOWN"), filename("NO_SUCH_FILE"), name("") {

    }*/

  /// Registry in which this should be saved
  std::string type;

  /// File in which this resource can be found
  std::string filename;

  /// Name of the resource in the file, can be empty if
  /// it does not apply.
  std::string name;

  static ResourceLocation parse(std::string type, std::string name);

  operator std::string() const {
    std::string res = filename;
    if (name != "") {
      res.append("::");
      res.append(name);
    }
    return res;
  }

};

template <> struct std::hash<ResourceLocation> {
  std::size_t operator()(ResourceLocation const & rl) const {
    std::size_t h1 = std::hash<std::string>{}(rl.filename);
    std::size_t h2 = std::hash<std::string>{}(rl.name);
    return h1 ^ (h2 << 1);
  }
};

inline bool operator==(const ResourceLocation & l1, const ResourceLocation & l2) {
  return l1.type == l2.type && l1.filename == l2.filename && l1.name == l2.name;
}

inline std::ostream & operator<<(std::ostream & stream, ResourceLocation loc) {
  stream << loc.filename;
  if (loc.name != "")
    stream << std::string("::") << loc.name;
  return stream;
}


#endif
