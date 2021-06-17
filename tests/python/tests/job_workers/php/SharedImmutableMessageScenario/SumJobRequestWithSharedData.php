<?php

namespace SharedImmutableMessageScenario;

class SumJobRequestWithSharedData implements \KphpJobWorkerRequest {
  /**
   * @var SharedMemoryData
   */
  public $shared_data = null;

  public $offset = 0;
  public $limit = 0;

  public function __construct(SharedMemoryData $shared_data, int $offset, int $limit) {
    $this->shared_data = $shared_data;
    $this->offset = $offset;
    $this->limit = $limit;
  }
}
