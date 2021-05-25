<?php

namespace ComplexScenario;

class CollectStatsJobRequest implements \KphpJobWorkerRequest {
  function __construct(int $master_port, StatsInterface $stat = null) {
    $this->master_port = $master_port;
    $this->stat = $stat;
  }

  /** @var int */
  public $master_port = 0;

  /** @var ?StatsInterface */
  public $stat = null;
}
