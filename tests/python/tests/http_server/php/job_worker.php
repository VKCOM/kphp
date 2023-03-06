<?php

function job_worker_main() {
  $req = kphp_job_worker_fetch_request();
  if ($req instanceof X2Request) {
    $x2_resp = new X2Response;
    foreach ($req->arr_request as $value) {
      $x2_resp->arr_reply[] = $value ** 2;
    }
    kphp_job_worker_store_response($x2_resp);
  } else {
    critical_error("Unexpected job " . get_class($req) . "\n");
  }
}

job_worker_main();
