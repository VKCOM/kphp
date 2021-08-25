<?php

use ReferenceInvariant\ReferenceInvariantRequest;
use ReferenceInvariant\ReferenceInvariantResponse;

function run_job_reference_invariant(ReferenceInvariantRequest $request) {
  $request->verify_data();
  $request->verify_job_ref_cnt();

  $resp = new ReferenceInvariantResponse();
  $resp->init_data();
  kphp_job_worker_store_response($resp);
}
