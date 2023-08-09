<?php

class X2Request implements KphpJobWorkerRequest {
  public $arr_request = [];
  public $tag = "";
  public $redirect_chain_len = 0;
  public $master_port = 0;
  public $sleep_time_sec = 0.0;
  public $error_type = "";
}
