@kphp_should_fail
/Class MySharedMemoryData must be immutable \(@kphp-immutable-class\) as it implements KphpJobWorkerSharedMemoryPiece/
<?php

class MySharedMemoryData implements KphpJobWorkerSharedMemoryPiece {
  public $arr = [];
}

