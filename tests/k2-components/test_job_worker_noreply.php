<?php

if (isset($_SERVER["JOB_ID"])) {
  $request = kphp_job_worker_fetch_request();
  if (kphp_job_worker_store_response($request . ": done!") == 0) {
    critical_error("unexpected success on store response");
  }
} else {
  $job = kphp_job_worker_start_no_reply("Job Request", 0.3);
  sched_yield_sleep(0.1);
}
