<?php

namespace SharedImmutableMessageScenario;

class SumJobRequestWithoutSharedData implements \KphpJobWorkerRequest {
  /**
   * @var int[]
   */
  public $arr = [];

  public $offset = 0;
  public $limit = 0;

  public function __construct($arr, int $offset, int $limit) {
    $this->arr = $arr;
    $this->offset = $offset;
    $this->limit = $limit;
  }
}
