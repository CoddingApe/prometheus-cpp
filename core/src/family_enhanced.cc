#include "prometheus/family_enhanced.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "prometheus/check_names.h"
#include "prometheus/counter_enhanced.h"
#include "prometheus/gauge_enhanced.h"
#include "prometheus/histogram_enhanced.h"
#include "prometheus/info.h"
#include "prometheus/labels.h"
#include "prometheus/summary.h"

namespace prometheus {

template <typename T>
FamilyEnhanced<T>::FamilyEnhanced(const std::string& name,
                                  const std::string& help,
                                  const Labels& constant_labels)
    : name(name), help(help), constant_labels(constant_labels) {
  if (!CheckMetricName(name)) {
    throw std::invalid_argument("Invalid metric name");
  }
  for (auto& label_pair : constant_labels) {
    auto& label_name = label_pair.first;
    if (!CheckLabelName(label_name, T::metric_type)) {
      throw std::invalid_argument("Invalid label name");
    }
  }
}

template <typename T>
std::shared_ptr<T> FamilyEnhanced<T>::add(const Labels& labels,
                                          std::shared_ptr<T> object) {
  auto&& [iter, inserted] = metrics.emplace(labels, object);
  return inserted ? object : iter->second;
}
template <typename T>
std::shared_ptr<T> FamilyEnhanced<T>::get(const Labels& labels) {
  auto&& iter = metrics.find(labels);
  if (iter == metrics.end()) {
    return nullptr;
  }

  return iter->second;
}

template <typename T>
void FamilyEnhanced<T>::remove(const Labels& labels) {
  metrics.erase(labels);
}

template <typename T>
const std::string& FamilyEnhanced<T>::get_name() const {
  return name;
}

template <typename T>
const Labels& FamilyEnhanced<T>::get_constant_labels() const {
  return constant_labels;
}

template <typename T>
std::vector<MetricFamily> FamilyEnhanced<T>::Collect(bool clear) const {
  if (metrics.empty()) {
    return {};
  }

  auto family = MetricFamily{};
  family.name = name;
  family.help = help;
  family.type = T::metric_type;
  family.metric.reserve(metrics.size());
  for (const auto& [labels, metric] : metrics) {
    family.metric.push_back(std::move(collect_metric(labels, metric)));
  }

  if (clear) {
    metrics.clear();
  }

  return {family};
}

template <typename T>
ClientMetric FamilyEnhanced<T>::collect_metric(
    const Labels& metric_labels, std::shared_ptr<T> metric) const {
  auto collected = metric->Collect();
  collected.label.reserve(constant_labels.size() + metric_labels.size());
  const auto add_label =
      [&collected](const std::pair<std::string, std::string>& label_pair) {
        auto label = ClientMetric::Label{};
        label.name = label_pair.first;
        label.value = label_pair.second;
        collected.label.push_back(std::move(label));
      };
  std::for_each(constant_labels.cbegin(), constant_labels.cend(), add_label);
  std::for_each(metric_labels.cbegin(), metric_labels.cend(), add_label);
  return collected;
}

template class PROMETHEUS_CPP_CORE_EXPORT FamilyEnhanced<CounterEnhanced>;
template class PROMETHEUS_CPP_CORE_EXPORT FamilyEnhanced<GaugeEnhanced>;
template class PROMETHEUS_CPP_CORE_EXPORT FamilyEnhanced<HistogramEnhanced>;
template class PROMETHEUS_CPP_CORE_EXPORT FamilyEnhanced<Info>;
template class PROMETHEUS_CPP_CORE_EXPORT FamilyEnhanced<Summary>;

}  // namespace prometheus
