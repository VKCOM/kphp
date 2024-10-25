<?php

if (isset($_SERVER["JOB_ID"])) {
  $request = job_worker_fetch_request();
  if (job_worker_store_response($request . ": done!") == 0) {
    critical_error("unexpected success on store response");
  }
} else {
  $job = job_worker_send_noreply_request("Job Request", 0.3);
  sched_yield_sleep(0.1);
}
