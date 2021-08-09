<?php

use ReferenceInvariant\ReferenceInvariantRequest;
use ReferenceInvariant\ReferenceInvariantResponse;

function run_job_reference_invariant(ReferenceInvariantRequest $request) {
  $request->verify_data();

  $resp = new ReferenceInvariantResponse();
  $resp->init_data();
  kphp_job_worker_store_response($resp);
}
