<?php

if (isset($_SERVER["JOB_ID"])) {
  $request = kphp_job_worker_fetch_request();
  sched_yield_sleep(0.1);
  kphp_job_worker_store_response($request . ": done!");
} else {
  $job = kphp_job_worker_start("Job Request", 0.3);
  if (wait($job) != "Job Request: done!") {
    critical_error("response not matched");
  }
}
