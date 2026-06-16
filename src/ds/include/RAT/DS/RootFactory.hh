#include <TClass.h>

#include <stdexcept>
#include <string>

#include "RAT/DS/Root.hh"

namespace RAT::DS {
class RootFactory {
 public:
  static void SetClassName(const std::string& name) { ClassName() = name; }

  static Root* Create() {
    const std::string& name = ClassName();
    if (name.empty() || name == "RAT::DS::Root") return new Root();  // default: base type

    TClass* cl = TClass::GetClass(name.c_str());
    if (!cl) throw std::runtime_error("RootFactory: no dictionary for " + name);
    if (!cl->InheritsFrom(Root::Class())) throw std::runtime_error("RootFactory: " + name + " is not a RAT::DS::Root");

    void* obj = cl->New();  // uses the I/O default ctor
    if (!obj) throw std::runtime_error("RootFactory: New() failed for " + name);

    // NOT static_cast<Root*>(obj) — see below
    return static_cast<Root*>(cl->DynamicCast(Root::Class(), obj, kTRUE));
  }

  static const std::string& GetClassName() { return ClassName(); }

 private:
  static std::string& ClassName() {
    static std::string n;
    return n;
  }
};
}  // namespace RAT::DS
