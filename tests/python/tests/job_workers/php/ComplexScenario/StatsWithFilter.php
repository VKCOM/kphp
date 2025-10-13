<?php

namespace ComplexScenario;

abstract class StatsWithFilter implements StatsInterface {
  /**
   * @param string $type
   * @param string[] $filter
   */
  function __construct(string $type, array $filter) {
    $this->filter = $filter;
    $this->type = $type;
  }

  /**
   * @var string[]
   */
  protected $filter = [];

  /** @var string */
  protected $type = "";
}
