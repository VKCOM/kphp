<?php

namespace ComplexScenario;

require_once "_stats_processor.php";

final class StatsRPCSimple extends StatsRPCBase {
  function __construct(array $filter) {
    parent::__construct("engine.stat", $filter);
  }

  /**
   * @return mixed[]
   */
  protected function make_request(): array {
    return ['_' => $this->type];
  }

  protected function make_stats(array $stats_raw): Stats {
    return parse_rpc_stats($stats_raw, $this->filter);
  }
}
