// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

struct StatsHouseMetric {
  int fields_mask{0};
  std::string name;
  //  std::vector<std::tuple<std::string, std::string>> tags; // not implemented yet
  double counter{0.0};           // fields_mask bit #0
  long long t{};                 // fields_mask bit #5
  std::vector<double> value;     // fields_mask bit #1
  std::vector<long long> unique; // fields_mask bit #2
  std::vector<std::string> stop; // fields_mask bit #3

  void tl_store() const;
};

struct StatsHouseAddMetricsBatch {
  int fields_mask{0};
  const std::vector<StatsHouseMetric> &metrics;

  void tl_store() const;
};

void add_statshouse_value_metric(std::vector<StatsHouseMetric> &metrics, std::string name, double value, long time);
