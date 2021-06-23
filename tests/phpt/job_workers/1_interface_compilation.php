@ok
<?php

function test_compilation($x) {
  if (!$x) {
    return;
  }

  interface AI {}

  class A1 implements AI {
    public $a1 = "a1";
  }

  class A2 implements AI {
    public $a2 = "a2";
  }

  class Req implements KphpJobWorkerRequest {
    /** @var AI */
    public $ai = null;
  }

  class Resp implements KphpJobWorkerResponse {
    /** @var AI */
    public $ai = null;
  }

  $m1 = new Req;
  $m1->$ai = new A1;
  kphp_job_worker_start($m1, -1);

  $m2 = new Req;
  $m2->$ai = new A2;
  kphp_job_worker_start($m2, -1);

  $x = kphp_job_worker_fetch_request();
  if ($x instanceof Req) {
    $m = $x->ai;
    if ($m instanceof A1) {
      var_dump($m->a1);
    }
  }

  $m3 = new Resp;
  $m3->$ai = new A1;
  kphp_job_worker_store_response($m3);

  $m4 = new Resp;
  $m4->$ai = new A2;
  kphp_job_worker_store_response($m4);
}

test_compilation(false);
