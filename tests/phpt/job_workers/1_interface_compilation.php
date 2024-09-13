@ok k2_skip
<?php

function test_compilation($should_start) {
  if (!$should_start) {
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

function test_multiple_inheritance($x) {
  if (!$x) {
    return;
  }

  interface ReqInterface {}
  abstract class ReqBaseClass implements \KphpJobWorkerRequest {}
  final class Request1 extends ReqBaseClass implements ReqInterface {}

  interface RespInterface {}
  abstract class RespBaseClass implements \KphpJobWorkerResponse {}
  final class Response1 extends RespBaseClass implements RespInterface {}

  kphp_job_worker_start(new Request1, -1);

  $x = kphp_job_worker_fetch_request();
  if ($x instanceof Response1) {
    var_dump(get_class($x));
  }
  kphp_job_worker_store_response(new Response1);
}

test_compilation(false);
test_multiple_inheritance(false);
