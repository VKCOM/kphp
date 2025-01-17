<?php

use SharedMemoryPieceCopying\JobRequest;
use SharedMemoryPieceCopying\JobResponse;
use SharedMemoryPieceCopying\SharedMemoryContext;
use SharedMemoryPieceCopying\SomeContext;

function test_shared_memory_piece_copying() {
  $data = json_decode(file_get_contents('php://input'));
  $label = (string)$data['label'];

  $some_data = [];
  $string = "";
  for ($i = 0; $i < 100; $i++) {
    $some_data[] = $i;
    $string = $string . "X";
  }
  $context = new SomeContext($some_data);

  $array = $some_data;
  $array[] = 101;

  $mixed = $string . "X";

  $shared_mem_context = new SharedMemoryContext($context, $array, $mixed, $string);
  $reqs = [];
  $jobs_num = get_job_workers_number();
  if ($label === "job_request:instance") {
    $jobs_num /= 2;
  }
  for ($i = 0; $i < $jobs_num; $i++) {
    $reqs[] = new JobRequest($shared_mem_context, $i, $label);
  }

  $job_ids = kphp_job_worker_start_multi($reqs, -1);

  $resumable_ids = [];
  foreach ($job_ids as $id) {
    if ($id) {
      $resumable_ids[] = $id;
    } else {
      raise_error("Some job id is false after start");
      return;
    }
  }

  $responses = wait_multi($resumable_ids);

  if ($label === "instance_cache:instance") {
    for ($i = 0; $i < $jobs_num; $i++) {
      /**
       * @var $instance SomeContext
       */
      $instance = instance_cache_fetch(SomeContext::class, "test_$i");
      if ($instance === null) {
        raise_error("Expected instance by key 'test_" . $i . "' hasn't been restored from instance cache");
      }
      for ($i = 0; $i < 100; $i++) {
        if ($instance->some_data[$i] !== $some_data[$i]) {
          raise_error("Data was changed in instance cache");
        }
      }
    }
  }

  $result = ['jobs-result' => []];
  foreach ($responses as $resp) {
    if ($resp instanceof JobResponse) {
      $result['jobs-result'][] = $resp->ans;
    } else {
      $result['jobs-result'][] = to_array_debug($resp);
    }
  }

  echo json_encode($result);
}
