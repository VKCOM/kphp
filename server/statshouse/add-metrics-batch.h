// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>
#include <vector>

struct StatsHouseMetric {
  int fields_mask{0};
  std::string name;
  const std::vector<std::pair<std::string, std::string>> &tags;
  double counter{0.0};           // fields_mask bit #0
  long long t{};                 // fields_mask bit #5
  std::vector<double> value;     // fields_mask bit #1
  std::vector<long long> unique; // fields_mask bit #2
  std::vector<std::string> stop; // fields_mask bit #3

  void tl_store() const;
};

struct StatsHouseAddMetricsBatch {
  int fields_mask{0};
  int metrics_size{0};

  void tl_store() const;
};

StatsHouseMetric make_statshouse_value_metric(std::string name, double value, const std::vector<std::pair<std::string, std::string>> &tags);
