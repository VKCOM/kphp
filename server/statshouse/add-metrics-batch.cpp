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
  tl_store_string(name.data(), name.size());
  tl_store_int(0); // tags size
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
  vk::tl::store_vector(metrics);
}

void add_statshouse_value_metric(std::vector<StatsHouseMetric> &metrics, std::string name, double value, long time) {
  constexpr int fields_mask = vk::tl::statshouse::metric_fields_mask::t | vk::tl::statshouse::metric_fields_mask::value;
  metrics.push_back({.fields_mask = fields_mask, .name = std::move(name), .t = time, .value = {value}});
}
