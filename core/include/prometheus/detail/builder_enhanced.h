#pragma once

#include <string>

#include "prometheus/labels.h"

// IWYU pragma: private
// IWYU pragma: no_include "prometheus/family.h"

namespace prometheus {

template <typename T>
class FamilyEnhanced;    // IWYU pragma: keep
class RegistryEnhanced;  // IWYU pragma: keep

namespace detail {

template <typename T>
class BuilderEnhanced {
 public:
  BuilderEnhanced& Labels(const ::prometheus::Labels& labels);
  BuilderEnhanced& Name(const std::string&);
  BuilderEnhanced& Help(const std::string&);
  void Register(RegistryEnhanced&);

 private:
  ::prometheus::Labels labels_;
  std::string name_;
  std::string help_;
};

}  // namespace detail
}  // namespace prometheus
