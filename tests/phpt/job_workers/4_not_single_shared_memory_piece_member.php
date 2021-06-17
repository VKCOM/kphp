@kphp_should_fail
/Class ExtendedJobRequest must have at most 1 member implementing KphpJobWorkerSharedMemoryPiece, but 2 found/
<?php

interface I {

}

/**
 * @kphp-immutable-class
 */
class MySharedMemoryData implements I, KphpJobWorkerSharedMemoryPiece {
  public $arr = [];
}

/**
 * @kphp-immutable-class
 */
class MySharedMemoryDataDerived extends MySharedMemoryData {
  public $val = 42;
}

class SimpleJobRequest implements KphpJobWorkerRequest {
 /**
  * @var MySharedMemoryData
  */
  public $x = null;
}

class ExtendedJobRequest extends SimpleJobRequest {
 /**
  * @var MySharedMemoryDataDerived
  */
  public $y = null;
}
