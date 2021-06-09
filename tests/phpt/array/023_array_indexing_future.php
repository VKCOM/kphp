@ok
<?php

class X2Request implements KphpJobWorkerRequest {
  public $arr_request = [];
  public $tag = "";
  public $master_port = 0;
  public $sleep_time_sec = 0;
  public $error_type = "";
}

function gather_jobs(array $ids, bool $use_wait = false): array {
  $result = [];
  foreach($ids as $id) {
    $resp = $use_wait ? wait($id) : kphp_job_worker_wait($id);
    if ($resp instanceof KphpJobWorkerResponseError) {
      $result[] = ["error" => $resp->getError(), "error_code" => $resp->getErrorCode()];
    }
  }
  return $result;
}

function send_jobs($context, float $timeout = -1.0): array {
  $ids = [];
  foreach ($context["data"] as $arr) {
    $req = new X2Request;
    $req->tag = (string)$context["tag"];
    $req->master_port = (int)$context["master-port"];
    $req->arr_request = (array)$arr;
    $req->sleep_time_sec = (int)$context["job-sleep-time-sec"];
    $req->error_type = (string)$context["error-type"];
    $id = kphp_job_worker_start($req, $timeout);
    if ($id) {
      $ids[] = $id;
    } else {
      critical_error("Can't send job");
    }
  }
  return $ids;
}

function f() {
  $context = ["data" => [1,2,3], "job-sleep-time-sec" => 1];
  $ids = send_jobs($context);

  $wait_queue = wait_queue_create($ids);

  $id = wait_queue_next($wait_queue, 0.2);
  if ($id !== false) {
    return;
  }

  $job_sleep_time = (float)$context["job-sleep-time-sec"];
  $result = [];
  while (!wait_queue_empty($wait_queue)) {
    $ready_id = wait_queue_next($wait_queue, $job_sleep_time + 0.2);
    if ($ready_id === false) {
      return;
    }
    $result[$ready_id] = gather_jobs([$ready_id], true)[0];
  }
}

f();
