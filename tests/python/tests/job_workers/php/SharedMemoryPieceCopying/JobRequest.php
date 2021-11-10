<?php


namespace SharedMemoryPieceCopying;


class JobRequest implements \KphpJobWorkerRequest {
  /**
   * @var SharedMemoryContext
   */
  public $shared_memory_context = null;
  public $id = 0;
  public $label = "";

  /**
   * JobRequest constructor.
   * @param SharedMemoryContext|null $shared_memory_context
   * @param int $id
   * @param string $label
   */
  public function __construct(?SharedMemoryContext $shared_memory_context, int $id, string $label)
  {
    $this->shared_memory_context = $shared_memory_context;
    $this->id = $id;
    $this->label = $label;
  }


}

