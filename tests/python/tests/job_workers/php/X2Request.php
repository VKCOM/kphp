<?php

class X2Request implements KphpJobWorkerRequest {
  public $arr_request = [];
  public $tag = "";
  public $master_port = 0;
  public $sleep_time_sec = 0;
  public $error_type = "";
}
