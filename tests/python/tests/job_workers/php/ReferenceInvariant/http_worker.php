<?php

use ReferenceInvariant\ReferenceInvariantRequest;
use ReferenceInvariant\ReferenceInvariantResponse;

function test_reference_invariant() {
  $req = new ReferenceInvariantRequest();
  $req->init_data();

  $job_id = kphp_job_worker_start($req, -1);
  if (!$job_id) {
    critical_error("Can't send job");
  }

  $result = wait($job_id);
  $result_stats = instance_cast($result, ReferenceInvariantResponse::class);
  if ($result_stats === null) {
    critical_error("got empty job response");
  }
  $result_stats->verify_data();
  $result_stats->verify_ref_cnt();
}
