<?php

if (isset($_SERVER["JOB_ID"])) {
  $request = kphp_job_worker_fetch_request();
  sched_yield_sleep(0.1);
  kphp_job_worker_store_response($request . ": done!");
} else {
  $job_arr = [];
  for ($i = 0; $i < 20; $i++) {
    $job_arr[] = "Job Request";
  }
  $job_ids = kphp_job_worker_start_multi($job_arr, 0.3);
  foreach ($job_ids as $id) {
    if (wait($id) != "Job Request: done!") {
      critical_error("response not matched");
    }
  }
}
