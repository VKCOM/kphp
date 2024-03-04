<?php

class X2Request implements KphpJobWorkerRequest {
  /**
   * @var int[]
   */
  public $arr_request = [];

  /**
   * @param int[] $arr_request
   */
  public function __construct(array $arr_request) {
    $this->arr_request = $arr_request;
  }
}
