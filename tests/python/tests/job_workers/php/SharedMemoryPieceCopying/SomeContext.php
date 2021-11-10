<?php

namespace SharedMemoryPieceCopying;

/**
 * @kphp-immutable-class
 */
class SomeContext {
  /**
   * @var int[]
   */
  public $some_data = [];

  /**
   * SomeContext constructor.
   * @param int[] $arr
   */
  public function __construct(array $arr)
  {
    $this->some_data = $arr;
  }


}
