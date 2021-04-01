<?php

namespace ComplexScenario;

class StatTraits {
  function __construct(array $stat_names, int $stat_type) {
    $this->stat_names = $stat_names;
    $this->stat_type = $stat_type;
  }

  /** @var string[] */
  public $stat_names = [];

  const MC_STATS = 1;
  const MC_STATS_FULL = 2;
  const MC_STATS_FAST = 3;
  const RPC_STATS = 4;
  const RPC_FILTERED_STATS = 5;

  /** @var int */
  public $stat_type = StatTraits::MC_STATS;
}
