<?php

namespace ComplexScenario;

require_once "_stats_processor.php";

abstract class StatsRPCBase extends StatsWithFilter {
  /**
   * @param string $type
   * @param string[] $filter
   */
  function __construct(string $type, array $filter) {
    parent::__construct($type, $filter);
  }

  /**
   * @return mixed[]
   */
  abstract protected function make_request(): array;

  abstract protected function make_stats(array $stats_raw): Stats;

  /**
   * @return mixed
   */
  final function start_processing(int $master_port) {
    return rpc_query($this->make_request(), $master_port);
  }

  /**
   * @param mixed $processing_id
   */
  final function finish_processing($processing_id): Stats {
    $result = rpc_tl_query_result_one($processing_id);
    return $this->make_stats($result);
  }
}
