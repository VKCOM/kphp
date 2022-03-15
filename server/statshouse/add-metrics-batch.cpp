// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/statshouse/add-metrics-batch.h"

#include <utility>

#include "common/tl/constants/statshouse.h"
#include "common/tl/parse.h"
#include "common/tl/store.h"

void StatsHouseMetric::tl_store() const {
  tl_store_int(fields_mask);
  vk::tl::store_string(name);
  tl_store_int(tags.size());
  for (const auto &tag : tags) {
    vk::tl::store_string(tag.first);
    vk::tl::store_string(tag.second);
  }
  if (fields_mask & vk::tl::statshouse::metric_fields_mask::counter) {
    tl_store_double(counter);
  }
  if (fields_mask & vk::tl::statshouse::metric_fields_mask::t) {
    tl_store_long(t);
  }
  if (fields_mask & vk::tl::statshouse::metric_fields_mask::value) {
    vk::tl::store_vector(value);
  }
  if (fields_mask & vk::tl::statshouse::metric_fields_mask::unique) {
    vk::tl::store_vector(unique);
  }
  if (fields_mask & vk::tl::statshouse::metric_fields_mask::stop) {
    vk::tl::store_vector(stop);
  }
}

void StatsHouseAddMetricsBatch::tl_store() const {
  tl_store_int(TL_STATSHOUSE_ADD_METRICS_BATCH);
  tl_store_int(fields_mask);
  tl_store_int(metrics_size);
}

StatsHouseMetric make_statshouse_value_metric(std::string &&name, double value, long long timestamp, const std::vector<std::pair<std::string, std::string>> &tags) {
  constexpr int fields_mask = vk::tl::statshouse::metric_fields_mask::value;
  return {.fields_mask = fields_mask, .name = std::move(name), .tags = tags, .counter = 0.0, .t = timestamp, .value = {value}};
}

StatsHouseMetric make_statshouse_value_metrics(std::string &&name, std::vector<double> &&value, long long timestamp, const std::vector<std::pair<std::string, std::string>> &tags) {
  constexpr int fields_mask = vk::tl::statshouse::metric_fields_mask::value;
  return {.fields_mask = fields_mask, .name = std::move(name), .tags = tags, .counter = 0.0, .t = timestamp, .value = std::move(value)};
}
