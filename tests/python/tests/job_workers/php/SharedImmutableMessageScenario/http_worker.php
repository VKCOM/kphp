<?php

use SharedImmutableMessageScenario\SharedMemoryData;
use SharedImmutableMessageScenario\SumJobRequestWithoutSharedData;
use SharedImmutableMessageScenario\SumJobRequestWithSharedData;

/**
 * @return tuple(int[], int, int, int[])
 */
function parse_input() {
  $context = json_decode(file_get_contents('php://input'));

  [$start, $end, $step] = $context['shared_data'];
  $batch_count = (int)$context['batch_count'];
  $limit = (int)$context['batch_size'];

  $arr = [];
  for ($i = (int)$start; $i < (int)$end; $i += (int)$step) {
    $arr[] = $i;
  }

  $error_indices = [];
  foreach ((array)$context['error_indices'] as $item) {
    $error_indices[] = (int)$item;
  }

  return tuple($arr, $batch_count, $limit, $error_indices);
}

function test_job_worker_start_multi_impl(bool $use_shared_data) {
  [$arr, $batch_count, $limit, $error_indices] = parse_input();

  $shared_data = null;
  if ($use_shared_data) {
    $shared_data = new SharedMemoryData($arr);
  }
  /**
   * @var \KphpJobWorkerRequest[]
   */
  $requests = [];
  $offset = 0;
  for ($i = 0; $i < $batch_count; $i++) {
    if (in_array($i, $error_indices)) {
      $cur_offset = -1;
      $cur_limit = -1;
    } else {
      $cur_offset = $offset;
      $cur_limit = $limit;
    }
    $requests[] = $use_shared_data ? new SumJobRequestWithSharedData($shared_data, $cur_offset, $cur_limit)
                                   : new SumJobRequestWithoutSharedData($arr, $cur_offset, $cur_limit);
    $offset += $limit;
  }

  $job_ids = kphp_job_worker_start_multi($requests, -1);
  foreach ($requests as $req) {
    if ($req instanceof SumJobRequestWithSharedData) {
      if ($req->shared_data !== $shared_data) {
        raise_error("Some shared memory instance has changed after kphp_job_worker_start_multi()");
        return;
      }
    }
  }

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

  $result = ['jobs-result' => []];
  foreach ($responses as $resp) {
    $result['jobs-result'][] = instance_to_array($resp);
  }

  echo json_encode($result);
}

function test_job_worker_start_multi_with_shared_data() {
  test_job_worker_start_multi_impl(true);
}

function test_job_worker_start_multi_without_shared_data() {
  test_job_worker_start_multi_impl(false);
}

function test_error_different_shared_memory_pieces() {
  [$arr, $batch_count, $limit] = parse_input();

  $shared_data = [new SharedMemoryData($arr), new SharedMemoryData($arr)];

  $requests = [];
  $offset = 0;
  for ($i = 0; $i < $batch_count; $i++) {
    $requests[] =new SumJobRequestWithSharedData($shared_data[$i % 2], $offset, $limit);
    $offset += $limit;
  }

  $job_ids = kphp_job_worker_start_multi($requests, -1);

  echo json_encode(['job-ids' => $job_ids]);
}

function test_job_worker_start_multi_with_errors() {
  test_job_worker_start_multi_impl(true);
}
