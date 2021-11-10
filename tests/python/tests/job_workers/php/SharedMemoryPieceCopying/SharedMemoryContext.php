<?php

namespace SharedMemoryPieceCopying;

/**
 * @kphp-immutable-class
 */
class SharedMemoryContext implements \KphpJobWorkerSharedMemoryPiece {
  /**
   * @var SomeContext
   */
  public $instance = null;

  /**
   * @var int[]
   */
  public $array = [];

  /**
   * @var mixed
   */
  public $mixed = null;
  public $string = "";

  /**
   * SharedMemoryContext constructor.
   * @param SomeContext|null $instance
   * @param int[] $array
   * @param mixed|null $mixed
   * @param string $string
   */
  public function __construct(?SomeContext $instance, array $array, $mixed, string $string)
  {
    $this->instance = $instance;
    $this->array = $array;
    $this->mixed = $mixed;
    $this->string = $string;
  }
}
