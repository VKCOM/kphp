<?php

if (isset($_SERVER["JOB_ID"])) {
  $request = job_worker_fetch_request();
  sched_yield_sleep(0.1);
  job_worker_store_response($request . ": done!");
} else {
  $job = job_worker_send_request("Job Request", 0.3);
  if (wait($job) != "Job Request: done!") {
    critical_error("response not matched");
  }
}
