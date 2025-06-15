#pragma once
#include <RAT/DB.hh>
#include <RAT/Log.hh>
#include <functional>
#include <map>
#include <memory>
#include <mimir/Common.hh>

// Generic factory for all MIMIR components.
namespace RAT::Mimir {

template <typename Base>
class Factory {
 public:
  using MakeFunc = std::function<std::unique_ptr<Base>()>;

  static Factory& GetInstance() {
    static Factory theInstance;
    return theInstance;
  }

  void register_type(const std::string& name, MakeFunc mf) { registry_map[name] = mf; }

  std::unique_ptr<Base> make(const std::string& type_name) const {
    auto it = registry_map.find(type_name);
    if (it == registry_map.end()) {
      RAT::Log::Die("[MIMIR Factory] Unknown type: " + type_name);
    }
    return it->second();
  }

  std::unique_ptr<Base> make_and_configure(const std::string& type_name, const std::string& config_name) const {
    std::unique_ptr<Base> instance = make(type_name);
    RAT::DBLinkPtr db_link = GetConfig(type_name, config_name);
    instance->SetName(GetConfigRepr(db_link));
    if (!instance->Configure(db_link)) {
      RAT::Log::Die("[MIMIR Factory] Failed to configure instance: " + instance->GetName());
    }
    return instance;
  }

 private:
  std::map<std::string, MakeFunc> registry_map;
};

}  // namespace RAT::Mimir

// Define a macro for the automatic registration of derived classes.
#define MIMIR_REGISTER_TYPE(BaseType, DerivedType, type_name)                                                  \
  namespace {                                                                                                  \
  static bool _reg_##DerivedType = [] {                                                                        \
    Factory<BaseType>::GetInstance().register_type(type_name, [] { return std::make_unique<DerivedType>(); }); \
    return true;                                                                                               \
  }();                                                                                                         \
  }
