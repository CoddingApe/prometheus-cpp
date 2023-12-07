#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "prometheus/collectable_enhanced.h"
#include "prometheus/detail/core_export.h"
#include "prometheus/labels.h"
#include "prometheus/metric_family.h"

namespace prometheus {

class CounterEnhanced;
class GaugeEnhanced;
class HistogramEnhanced;
class Info;
class Summary;

template <typename T>
class FamilyEnhanced;

namespace detail {

template <typename T>
class BuilderEnhanced;  // IWYU pragma: keep

}

template <typename T>
using FamilyMap =
    std::unordered_map<std::string, std::shared_ptr<FamilyEnhanced<T>>>;

template <typename T>
using FamilyVec = std::vector<std::unique_ptr<FamilyEnhanced<T>>>;

/// \brief Manages the collection of a number of metrics.
///
/// The RegistryImproved is responsible to expose data to a
/// class/method/function "bridge", which returns the metrics in a format
/// Prometheus supports.
///
/// The key class is the Collectable. This has a method - called Collect() -
/// that returns zero or more metrics and their samples. The metrics are
/// represented by the class Family<>, which implements the Collectable
/// interface. A new metric is registered with BuildCounter(), BuildGauge(),
/// BuildHistogram(), BuildInfo() or BuildSummary().
///
/// The class is thread-safe. No concurrent call to any API of this type causes
/// a data race.
class PROMETHEUS_CPP_CORE_EXPORT RegistryEnhanced : public CollectableEnhanced {
 public:
  /// \brief How to deal with repeatedly added family names for a type.
  ///
  /// Adding a family with the same name but different types is always an error
  /// and will lead to an exception.
  enum class InsertBehavior {
    /// \brief If a family with the same name and labels already exists return
    /// the existing one. If no family with that name exists create it.
    /// Otherwise throw.
    Merge,
    /// \brief Throws if a family with the same name already exists.
    Throw,
  };

  /// \brief name Create a new RegistryImproved.
  ///
  /// \param insert_behavior How to handle families with the same name.
  explicit RegistryEnhanced(
      InsertBehavior insert_behavior = InsertBehavior::Merge);

  /// \brief Deleted copy constructor.
  RegistryEnhanced(const RegistryEnhanced&) = delete;

  /// \brief Deleted copy assignment.
  RegistryEnhanced& operator=(const RegistryEnhanced&) = delete;

  /// \brief Deleted move constructor.
  RegistryEnhanced(RegistryEnhanced&&) = delete;

  /// \brief Deleted move assignment.
  RegistryEnhanced& operator=(RegistryEnhanced&&) = delete;

  /// \brief name Destroys a RegistryImproved.
  ~RegistryEnhanced() override;

  /// \brief Returns a list of metrics and their samples.
  ///
  /// Every time the RegistryImproved is scraped it calls each of the metrics
  /// Collect function.
  ///
  /// \return Zero or more metrics and their samples.
  std::vector<MetricFamily> Collect(bool clean) const override;

  /// \brief Removes a metrics family from the RegistryImproved.
  ///
  /// Please note that this operation invalidates the previously
  /// returned reference to the Family and all of their added
  /// metric objects.
  ///
  /// \tparam T One of the metric types Counter, Gauge, Histogram or Summary.
  /// \param family The family to remove
  ///
  /// \return True if the family was found and removed.
  template <typename T>
  bool remove(const std::string& name, const Labels& labels);

  template <typename T, typename... Args>
  void inc(uint64_t value, const std::string& name, const Labels& labels,
           Args&&... args);

  template <typename T, typename... Args>
  void dec(uint64_t value, const std::string& name, const Labels& labels,
           Args&&... args);

  template <typename T, typename... Args>
  void set(uint64_t value, const std::string& name, const Labels& labels,
           Args&&... args);

  template <typename T, typename... Args>
  void observe(uint64_t value, const std::string& name, const Labels& labels,
               Args&&... args);

 private:
  template <typename T>
  friend class detail::BuilderEnhanced;

  template <typename T>
  FamilyMap<T>& get_families();

  template <typename T>
  bool family_conflict(const std::string& name) const;

  template <typename T>
  std::shared_ptr<FamilyEnhanced<T>> add_family(const std::string& name,
                                                const std::string& help,
                                                const Labels& labels);
  template <typename T>
  std::shared_ptr<FamilyEnhanced<T>> get_family(const std::string& name);

  template <typename T, typename... Args>
  std::shared_ptr<T> get_or_add_metric(const std::string& name,
                                       const Labels& labels, Args&&... args);

  const InsertBehavior insert_behavior_;
  FamilyMap<CounterEnhanced> counters_map;
  FamilyMap<GaugeEnhanced> gauges_map;
  FamilyMap<HistogramEnhanced> histograms_map;
  FamilyMap<Info> infos_map;
  FamilyMap<Summary> summaries_map;
  mutable std::shared_mutex rw_lock;
};

}  // namespace prometheus
