#pragma once

namespace prometheus {

enum class MetricType {
  Counter,
  CounterEnhanced,
  Gauge,
  GaugeEnhanced,
  Summary,
  Untyped,
  Histogram,
  HistogramEnhanced,
  Info,
};

}  // namespace prometheus
