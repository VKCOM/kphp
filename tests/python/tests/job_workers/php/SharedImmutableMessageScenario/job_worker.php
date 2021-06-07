<?php

use SharedImmutableMessageScenario\SumJobRequestWithoutSharedData;
use SharedImmutableMessageScenario\SumJobRequestWithSharedData;
use SharedImmutableMessageScenario\SumJobResponse;

function run_job_shared_immutable_message_scenario(KphpJobWorkerRequest $req)
{
  $sum = 0;

  if ($req instanceof SumJobRequestWithSharedData) {
    if ($req->limit == -1 && $req->offset == -1) {
      critical_error("Test php_assert");
      return;
    }
    for ($i = 0; $i < $req->limit; $i++) {
      $sum += $req->shared_data->arr[$req->offset + $i];
    }
  } else if ($req instanceof SumJobRequestWithoutSharedData) {
    for ($i = 0; $i < $req->limit; $i++) {
      $sum += $req->arr[$req->offset + $i];
    }
  } else {
    critical_error("Unknown job request");
  }

  kphp_job_worker_store_response(new SumJobResponse($sum));
}
