<?php

namespace SharedImmutableMessageScenario;

class SumJobResponse implements \KphpJobWorkerResponse
{
  public $range_sum = 0;

  public function __construct(int $range_sum) {
    $this->range_sum = $range_sum;
  }
}
