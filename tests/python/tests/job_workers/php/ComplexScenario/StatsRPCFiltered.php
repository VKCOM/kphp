<?php

namespace ComplexScenario;

require_once "_stats_processor.php";

final class StatsRPCFiltered extends StatsRPCBase {
  /**
   * @param string[] $filter
   */
  function __construct(array $filter) {
    parent::__construct("engine.filteredStat", []);
    $this->filter_for_engine = $filter;
  }

  /**
   * @var string[]
   */
  private $filter_for_engine = [];

  /**
   * @return mixed[]
   */
  protected function make_request(): array {
    return ["_" => $this->type, "stat_names" => $this->filter_for_engine];
  }

  protected function make_stats(array $stats_raw): Stats {
    return parse_rpc_stats($stats_raw);
  }
}
