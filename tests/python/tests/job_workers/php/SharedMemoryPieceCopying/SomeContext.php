<?php

namespace SharedMemoryPieceCopying;

/**
 * @kphp-immutable-class
 *  @kphp-serializable
 */
class SomeContext {
  /**
   * @var int[]
   * @kphp-serialized-field 0
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
