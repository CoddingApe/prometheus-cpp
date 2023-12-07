#include "prometheus/gauge_enhanced.h"

#include <ctime>

namespace prometheus {

GaugeEnhanced::GaugeEnhanced(const uint64_t value) : atomic_value{value} {}

void GaugeEnhanced::Increment() { Increment(1); }

void GaugeEnhanced::Increment(const uint64_t value) { Change(value); }

void GaugeEnhanced::Decrement() { Decrement(1); }

void GaugeEnhanced::Decrement(const uint64_t v) { Change(-1.0 * v); }

void GaugeEnhanced::Set(const uint64_t value) { atomic_value.store(value); }

void GaugeEnhanced::Change(const uint64_t value) {
  atomic_value.fetch_add(value);
}

void GaugeEnhanced::SetToCurrentTime() {
  const auto time = std::time(nullptr);
  Set(static_cast<double>(time));
}

double GaugeEnhanced::Value() const { return atomic_value; }

ClientMetric GaugeEnhanced::Collect() const {
  ClientMetric metric;
  metric.gauge.value = Value();
  return metric;
}

}  // namespace prometheus
