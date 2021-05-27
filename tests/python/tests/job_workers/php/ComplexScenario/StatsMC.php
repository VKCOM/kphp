<?php

namespace ComplexScenario;

require_once "_stats_processor.php";

final class StatsMC extends StatsWithFilter {
  const DEFAULT_TYPE = "stats";
  const FULL_TYPE = "stats_full";
  const FAST_TYPE = "stats_fast";

  /**
   * @param string $type
   * @param string[] $filter
   */
  function __construct(string $type, array $filter) {
    parent::__construct($type, $filter);
  }

  /**
   * @return mixed
   */
  function start_processing(int $master_port) {
    return mc_get($this->type, $master_port);
  }

  /**
   * @param mixed $processing_id
   */
  function finish_processing($processing_id): Stats {
    return parse_mc_stats((string)$processing_id, $this->filter);
  }
}
