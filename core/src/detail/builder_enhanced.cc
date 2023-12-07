#include "prometheus/detail/builder_enhanced.h"

#include "prometheus/counter_enhanced.h"
#include "prometheus/detail/core_export.h"
#include "prometheus/gauge_enhanced.h"
#include "prometheus/histogram_enhanced.h"
#include "prometheus/registry_enhanced.h"

namespace prometheus {

namespace detail {

template <typename T>
BuilderEnhanced<T>& BuilderEnhanced<T>::Labels(
    const ::prometheus::Labels& labels) {
  labels_ = labels;
  return *this;
}

template <typename T>
BuilderEnhanced<T>& BuilderEnhanced<T>::Name(const std::string& name) {
  name_ = name;
  return *this;
}

template <typename T>
BuilderEnhanced<T>& BuilderEnhanced<T>::Help(const std::string& help) {
  help_ = help;
  return *this;
}

template <typename T>
void BuilderEnhanced<T>::Register(RegistryEnhanced& registry) {
  registry.add_family<T>(name_, help_, labels_);
}

template class PROMETHEUS_CPP_CORE_EXPORT BuilderEnhanced<CounterEnhanced>;
template class PROMETHEUS_CPP_CORE_EXPORT BuilderEnhanced<GaugeEnhanced>;
template class PROMETHEUS_CPP_CORE_EXPORT BuilderEnhanced<HistogramEnhanced>;

}  // namespace detail

detail::BuilderEnhanced<CounterEnhanced> BuildCounterEnhanced() { return {}; }
detail::BuilderEnhanced<GaugeEnhanced> BuildGaugeEnhanced() { return {}; }
detail::BuilderEnhanced<HistogramEnhanced> BuildHistogramEnhanced() { return {}; }

}  // namespace prometheus
