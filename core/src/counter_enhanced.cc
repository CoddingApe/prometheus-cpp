#include "prometheus/counter_enhanced.h"

namespace prometheus {

void CounterEnhanced::Increment() { gauge_.Increment(); }

void CounterEnhanced::Increment(const uint64_t val) {
  if (val > 0) {
    gauge_.Increment(val);
  }
}

double CounterEnhanced::Value() const { return gauge_.Value(); }

void CounterEnhanced::Reset() { gauge_.Set(0); }

ClientMetric CounterEnhanced::Collect() const {
  ClientMetric metric;
  metric.counter.value = Value();
  return metric;
}

}  // namespace prometheus
