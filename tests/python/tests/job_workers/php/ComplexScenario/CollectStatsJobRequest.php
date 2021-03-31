<?php

namespace ComplexScenario;

class CollectStatsJobRequest implements \KphpJobWorkerRequest {
  function __construct(int $master_port, StatTraits $traits = null) {
    $this->master_port = $master_port;
    $this->traits = $traits;
  }

  /** @var int */
  public $master_port = 0;

  /** @var ?StatTraits */
  public $traits = null;
}
