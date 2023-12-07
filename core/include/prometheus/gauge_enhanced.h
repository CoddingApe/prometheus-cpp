#pragma once

#include <atomic>

#include "prometheus/client_metric.h"
#include "prometheus/detail/builder.h"  // IWYU pragma: export
#include "prometheus/detail/builder_enhanced.h"
#include "prometheus/detail/core_export.h"
#include "prometheus/metric_type.h"

namespace prometheus {

/// \brief A gauge metric to represent a value that can arbitrarily go up and
/// down.
///
/// The class represents the metric type gauge:
/// https://prometheus.io/docs/concepts/metric_types/#gauge
///
/// Gauges are typically used for measured values like temperatures or current
/// memory usage, but also "counts" that can go up and down, like the number of
/// running processes.
///
/// The class is thread-safe. No concurrent call to any API of this type causes
/// a data race.
class PROMETHEUS_CPP_CORE_EXPORT GaugeEnhanced {
 public:
  static const MetricType metric_type{MetricType::GaugeEnhanced};

  /// \brief Create a gauge that starts at 0.
  GaugeEnhanced() = default;

  /// \brief Create a gauge that starts at the given amount.
  explicit GaugeEnhanced(uint64_t);

  /// \brief Increment the gauge by 1.
  void Increment();

  /// \brief Increment the gauge by the given amount.
  void Increment(uint64_t);

  /// \brief Decrement the gauge by 1.
  void Decrement();

  /// \brief Decrement the gauge by the given amount.
  void Decrement(uint64_t);

  /// \brief Set the gauge to the given value.
  void Set(uint64_t);

  /// \brief Set the gauge to the current unix time in seconds.
  void SetToCurrentTime();

  /// \brief Get the current value of the gauge.
  double Value() const;

  /// \brief Get the current value of the gauge.
  ///
  /// Collect is called by the Registry when collecting metrics.
  ClientMetric Collect() const;

 private:
  void Change(uint64_t);
  std::atomic<uint64_t> atomic_value{0};
};

/// \brief Return a builder to configure and register a Gauge metric.
///
/// @copydetails Family<>::Family()
///
/// Example usage:
///
/// \code
/// auto registry = std::make_shared<Registry>();
/// auto& gauge_family = prometheus::BuildGauge()
///                          .Name("some_name")
///                          .Help("Additional description.")
///                          .Labels({{"key", "value"}})
///                          .Register(*registry);
///
/// ...
/// \endcode
///
/// \return An object of unspecified type T, i.e., an implementation detail
/// except that it has the following members:
///
/// - Name(const std::string&) to set the metric name,
/// - Help(const std::string&) to set an additional description.
/// - Labels(const Labels&) to assign a set of
///   key-value pairs (= labels) to the metric.
///
/// To finish the configuration of the Gauge metric register it with
/// Register(Registry&).
PROMETHEUS_CPP_CORE_EXPORT detail::BuilderEnhanced<GaugeEnhanced> BuildGaugeEnhanced();

}  // namespace prometheus