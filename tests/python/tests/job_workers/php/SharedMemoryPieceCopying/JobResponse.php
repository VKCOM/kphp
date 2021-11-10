<?php


namespace SharedMemoryPieceCopying;


class JobResponse implements \KphpJobWorkerResponse
{
  public int $ans = 0;

  /**
   * @var SomeContext
   */
  public $instance_from_shared_mem = null;

  /**
   * @var int[]
   */
  public $array_from_shared_mem = [];

  /**
   * @var mixed
   */
  public $mixed_from_shared_mem = null;
  public $string_from_shared_mem = "";
}
