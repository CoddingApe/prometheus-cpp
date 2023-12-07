#include "prometheus/registry_enhanced.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>

#include "prometheus/counter_enhanced.h"
#include "prometheus/detail/future_std.h"
#include "prometheus/family_enhanced.h"
#include "prometheus/gauge_enhanced.h"
#include "prometheus/histogram_enhanced.h"
#include "prometheus/info.h"
#include "prometheus/summary.h"

namespace prometheus {

namespace {
template <typename T>
void CollectAll(std::vector<MetricFamily>& results, const T& families, bool clean) {
  for (auto&& [_, collectable] : families) {
    auto metrics = collectable->Collect(clean);
    results.insert(results.end(), std::make_move_iterator(metrics.begin()),
                   std::make_move_iterator(metrics.end()));
  }
}

bool family_name_exists(const std::string& /* name */) { return false; }

template <typename T, typename... Args>
bool family_name_exists(const std::string& name, const T& families,
                        Args&&... args) {
  auto exists = families.count(name) > 0;
  return exists || family_name_exists(name, args...);
}

template <typename T>
void swap_families(FamilyMap<T>& f1, FamilyMap<T> f2) {
  std::swap(f1, f2);
}
}  // namespace

RegistryEnhanced::RegistryEnhanced(InsertBehavior insert_behavior)
    : insert_behavior_{insert_behavior} {}

RegistryEnhanced::~RegistryEnhanced() = default;

std::vector<MetricFamily> RegistryEnhanced::Collect(bool clean) const {
  std::unique_lock<std::shared_mutex> write_lock(rw_lock);
  auto results = std::vector<MetricFamily>{};
  CollectAll(results, counters_map, clean);
  CollectAll(results, gauges_map, clean);
  CollectAll(results, histograms_map, clean);
  CollectAll(results, summaries_map, clean);
  CollectAll(results, infos_map, clean);

  return results;
}

template <>
FamilyMap<CounterEnhanced>& RegistryEnhanced::get_families() {
  return counters_map;
}

template <>
FamilyMap<GaugeEnhanced>& RegistryEnhanced::get_families() {
  return gauges_map;
}

template <>
FamilyMap<HistogramEnhanced>& RegistryEnhanced::get_families() {
  return histograms_map;
}

template <>
FamilyMap<Info>& RegistryEnhanced::get_families() {
  return infos_map;
}

template <>
FamilyMap<Summary>& RegistryEnhanced::get_families() {
  return summaries_map;
}

template <>
bool RegistryEnhanced::family_conflict<CounterEnhanced>(
    const std::string& name) const {
  return family_name_exists(name, gauges_map, histograms_map, infos_map,
                            summaries_map);
}

template <>
bool RegistryEnhanced::family_conflict<GaugeEnhanced>(
    const std::string& name) const {
  return family_name_exists(name, counters_map, histograms_map, infos_map,
                            summaries_map);
}

template <>
bool RegistryEnhanced::family_conflict<HistogramEnhanced>(
    const std::string& name) const {
  return family_name_exists(name, counters_map, gauges_map, infos_map,
                            summaries_map);
}

template <>
bool RegistryEnhanced::family_conflict<Info>(const std::string& name) const {
  return family_name_exists(name, counters_map, gauges_map, histograms_map,
                            summaries_map);
}

template <>
bool RegistryEnhanced::family_conflict<Summary>(const std::string& name) const {
  return family_name_exists(name, counters_map, gauges_map, histograms_map,
                            infos_map);
}

template <typename T>
std::shared_ptr<FamilyEnhanced<T>> RegistryEnhanced::add_family(
    const std::string& name, const std::string& help, const Labels& labels) {
  std::shared_lock<std::shared_mutex> read_lock(rw_lock);
  std::shared_ptr<FamilyEnhanced<T>> existed = get_family<T>(name);
  if (nullptr != existed) {
    return existed;
  }
  read_lock.unlock();

  std::unique_lock<std::shared_mutex> write_lock(rw_lock);
  auto& families = get_families<T>();
  auto family = detail::make_shared<FamilyEnhanced<T>>(name, help, labels);
  auto&& [iter, _] = families.emplace(name, std::move(family));
  return iter->second;
}

template <typename T>
std::shared_ptr<FamilyEnhanced<T>> RegistryEnhanced::get_family(
    const std::string& name) {
  auto& families = get_families<T>();
  auto&& iter = families.find(name);
  if (iter != families.end()) {
    return iter->second;
  }
  return nullptr;
}

template <typename T, typename... Args>
std::shared_ptr<T> RegistryEnhanced::get_or_add_metric(const std::string& name,
                                                       const Labels& labels,
                                                       Args&&... args) {
  std::shared_lock<std::shared_mutex> read_lock(rw_lock);
  auto& families = get_families<T>();
  auto&& family_iter = families.find(name);
  if (family_iter == families.end()) {
    std::stringstream ss;
    ss << "Family " << name << " not initialized before using it" << std::endl;
    throw std::logic_error(ss.str());
  }

  auto&& family = family_iter->second;
  auto&& metric = family->get(labels);
  if (nullptr != metric) {
    return metric;
  }
  read_lock.unlock();

  std::shared_lock<std::shared_mutex> write_lock(rw_lock);
  return family->Add(labels, args...);
}

template <typename T>
bool RegistryEnhanced::remove(const std::string& name, const Labels& labels) {
  std::shared_lock<std::shared_mutex> read_lock(rw_lock);
  auto& families = get_families<T>();
  auto&& family_iter = families.find(name);
  if (family_iter != families.end()) {
    auto& family = family_iter->second;
    read_lock.unlock();
    family->remove(labels);
  }
  return true;
}

template <typename T, typename... Args>
void RegistryEnhanced::inc(uint64_t value, const std::string& name,
                           const Labels& labels, Args&&... args) {
  std::shared_ptr<T> metric = get_or_add_metric<T>(name, labels, args...);
  metric->Increment(value);
}

template <typename T, typename... Args>
void RegistryEnhanced::dec(uint64_t value, const std::string& name,
                           const Labels& labels, Args&&... args) {
  std::shared_ptr<T> metric = get_or_add_metric<T>(name, labels, args...);
  metric->Decrement(value);
}

template <typename T, typename... Args>
void RegistryEnhanced::set(uint64_t value, const std::string& name,
                           const Labels& labels, Args&&... args) {
  std::shared_ptr<T> metric = get_or_add_metric<T>(name, labels, args...);
  metric->Set(value);
}

template <typename T, typename... Args>
void RegistryEnhanced::observe(uint64_t value, const std::string& name,
                               const Labels& labels, Args&&... args) {
  std::shared_ptr<T> metric = get_or_add_metric<T>(name, labels, args...);
  metric->Observe(value);
}

template std::shared_ptr<FamilyEnhanced<CounterEnhanced>>
    PROMETHEUS_CPP_CORE_EXPORT RegistryEnhanced::add_family<CounterEnhanced>(
        const std::string& name, const std::string& help, const Labels& labels);

template std::shared_ptr<FamilyEnhanced<GaugeEnhanced>>
    PROMETHEUS_CPP_CORE_EXPORT RegistryEnhanced::add_family<GaugeEnhanced>(
        const std::string& name, const std::string& help, const Labels& labels);

template std::shared_ptr<FamilyEnhanced<HistogramEnhanced>>
    PROMETHEUS_CPP_CORE_EXPORT RegistryEnhanced::add_family<HistogramEnhanced>(
        const std::string& name, const std::string& help, const Labels& labels);

template void PROMETHEUS_CPP_CORE_EXPORT RegistryEnhanced::inc<CounterEnhanced>(
    uint64_t value, const std::string& name, const Labels& labels);

template void PROMETHEUS_CPP_CORE_EXPORT RegistryEnhanced::inc<GaugeEnhanced>(
    uint64_t value, const std::string& name, const Labels& labels);

template void PROMETHEUS_CPP_CORE_EXPORT RegistryEnhanced::dec<GaugeEnhanced>(
    uint64_t value, const std::string& name, const Labels& labels);

template void PROMETHEUS_CPP_CORE_EXPORT RegistryEnhanced::set<GaugeEnhanced>(
    uint64_t value, const std::string& name, const Labels& labels);

template void PROMETHEUS_CPP_CORE_EXPORT
RegistryEnhanced::observe<HistogramEnhanced>(
    uint64_t value, const std::string& name, const Labels& labels,
    const HistogramEnhanced::BucketBoundaries& b);

template bool PROMETHEUS_CPP_CORE_EXPORT
RegistryEnhanced::remove<CounterEnhanced>(const std::string& name,
                                          const Labels& labels);

template bool PROMETHEUS_CPP_CORE_EXPORT
RegistryEnhanced::remove<GaugeEnhanced>(const std::string& name,
                                        const Labels& labels);

template bool PROMETHEUS_CPP_CORE_EXPORT
RegistryEnhanced::remove<HistogramEnhanced>(const std::string& name,
                                            const Labels& labels);

}  // namespace prometheus
