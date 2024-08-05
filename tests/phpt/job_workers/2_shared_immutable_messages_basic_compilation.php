@ok k2_skip
<?php

function test_compilation($x) {
  if (!$x) {
    return;
  }

  /**
   * @kphp-immutable-class
   */
  class MySharedMemoryData implements KphpJobWorkerSharedMemoryPiece {
    public $arr = [];

    public function __construct(array $arr) {
        $this->arr = $arr;
    }
  }

  class SimpleJobRequest implements KphpJobWorkerRequest {
   /**
    * @var MySharedMemoryData
    */
    public $shared_data = null;

    public $offset = 0;
    public $limit = 0;

    public function __construct(MySharedMemoryData $shared_data, int $offset, int $limit) {
        $this->shared_data = $shared_data;
        $this->offset = $offset;
        $this->limit = $limit;
    }
  }

  $total_count = 1000;
  $limit = 100;
  $arr = [];
  for ($i = 0; $i < $total_count; ++$i) {
    $arr[] = $i;
  }

  $shared_data = new MySharedMemoryData($arr);
  $requests = [];
  for ($offset = 0; $offset < $total_count; $offset += $limit) {
  	$requests[] = new SimpleJobRequest($shared_data, $offset, $limit);
  }

  kphp_job_worker_start_multi($requests, -1);
}

test_compilation(false);
