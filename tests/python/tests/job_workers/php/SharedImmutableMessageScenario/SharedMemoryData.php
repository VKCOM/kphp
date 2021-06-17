<?php

namespace SharedImmutableMessageScenario;

/**
 * @kphp-immutable-class
 */
class SharedMemoryData implements \KphpJobWorkerSharedMemoryPiece {
  /**
   * @var int[]
   */
  public $arr = [];

  /**
   * SharedMemoryData constructor.
   * @param int[] $arr
   */
  public function __construct($arr) {
    $this->arr = $arr;
  }
}
