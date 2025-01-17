<?php

use SharedMemoryPieceCopying\JobRequest;
use SharedMemoryPieceCopying\JobResponse;

function run_shared_memory_piece_copying_job(JobRequest $req) {
  $resp = new JobResponse();

  $resp->ans = 42;

  switch ($req->label) {
    case "job_response:instance": {
      $resp->instance_from_shared_mem = $req->shared_memory_context->instance;
      break;
    }
    case "job_response:array": {
      $resp->array_from_shared_mem = $req->shared_memory_context->array;
      break;
    }
    case "job_response:mixed": {
      $resp->mixed_from_shared_mem = $req->shared_memory_context->mixed;
      break;
    }
    case "job_response:string": {
      $resp->string_from_shared_mem = $req->shared_memory_context->string;
      break;
    }
    case "job_request:instance": {
      $req->label = "";
      $inner_req_id = kphp_job_worker_start($req, -1);
      if ($inner_req_id) {
         $inner_resp = wait($inner_req_id);
      }
      break;
    }
    case "instance_cache:instance": {
      // The Instance Cache does not guarantee immediate and in-place data storage.
      // To avoid flakiness, we need to ensure that key-value pairs are stored.
      // Attempt storage for up to 1 second.
      $t = time();
      while (!instance_cache_store("test_" . $req->id, $req->shared_memory_context->instance) && time() - $t <= 1) {}
      break;
    }
  }

  kphp_job_worker_store_response($resp);
}
