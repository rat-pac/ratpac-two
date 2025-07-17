#pragma once

#include <RAT/DB.hh>

namespace RAT::Mimir {
constexpr int INVALID = -9999;

inline RAT::DBLinkPtr GetConfig(const std::string& type_name, const std::string& index = "") {
  return RAT::DB::Get()->GetLink("MIMIR_" + type_name, index);
}

inline std::string GetConfigRepr(RAT::DBLinkPtr db_link) {
  std::stringstream result;
  result << db_link->GetName() << "[" << db_link->GetIndex() << "]";
  return result.str();
}

}  // namespace RAT::Mimir
