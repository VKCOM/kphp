<?php

function make_array_of_volume(int $bytes) {
  $arr = [];
  $size = $bytes / 8;
  array_reserve_vector($arr, $size);
  for ($i = 0; $i < $size; $i++) {
    $arr[$i] = $i;
  }
  return $arr;
}


function safe_sleep(float $sleep_time_sec) {
  $s = microtime(true);
  do {
  } while(microtime(true) - $s < $sleep_time_sec);
}


function make_oom_error() {
  make_array_of_volume(11 * 1024 * 1024);
}

function make_timeout_error() {
  safe_sleep(2);
}

function fake_query(float $time_to_sleep) {
  sched_yield_sleep($time_to_sleep);
  return true;
}

$script_allocated_array = [];

/**
 * @kphp-required
 */
function oom_handler() {
  global $script_allocated_array, $test_case;

  register_shutdown_function(function() {
    critical_error("Shutdown functions registered from OOM handler shouldn't be called too");
  });

  $fork_id = (int)get_running_fork_id();
  fprintf(STDERR, "start OOM handler from fork_id=$fork_id\n");

  [$allocations_cnt_start, $mem_allocated_start] = memory_get_allocations();
  $arr = [];
  array_reserve_vector($arr, 100000);
  for ($i = 0; $i < 100000; $i++) {
    $arr[] = $i;
  }

  if ($test_case === "oom_inside_oom_handler") {
    make_oom_error();
  } else if ($test_case === "with_rpc_request") {
    $master_port = (int)$_GET["master_port"];
    $conn = new_rpc_connection('127.0.0.1', $master_port, 0, 5);
    $req_id = rpc_tl_query_one($conn, ["_" => "engine.sleep", "time_ms" => 100]);
    $resp = rpc_tl_query_result_one($req_id);
    $result = $resp["result"];
    fprintf(STDERR, "rpc_request_succeeded=$result\n");
  } else if ($test_case === "with_job_request") {
    $req_arr = [1,2,3,4,5,6,7,8,9];
    $i = new X2Request($req_arr);
    $future = kphp_job_worker_start($i, 1);
    $resp = wait($future);
    if ($resp instanceof X2Response) {
      for ($i = 0; $i < count($req_arr); $i++) {
        $req_arr[$i] **= 2;
      }
      $diff = array_diff($req_arr, $resp->arr_reply);
      $success = count($diff) === 0;
      fprintf(STDERR, "job_request_succeeded=$success\n");
    }
  } else if ($test_case === "with_instance_cache") {
    $success = instance_cache_store("oom_handler_test_key", new A);
    fprintf(STDERR, "instance_cache_store_succeeded=$success\n");
  } else if ($test_case === "script_memory_dealloc") {
    $refcnt = get_reference_counter($script_allocated_array);
    fprintf(STDERR, "refcnt_before_unset=$refcnt\n");
    $script_allocated_array = [];
  } else if ($test_case === "script_memory_realloc") {
    [$realloc_allocations_cnt_start, $realloc_mem_allocated_start] = memory_get_allocations();
    $len = count($script_allocated_array);
    for ($i = 0; $i < 2 * $len; $i++) {
       $script_allocated_array[] = 1000 + $i;
    }
    [$realloc_allocations_cnt_end, $realloc_mem_allocated_end] = memory_get_allocations();
    $realloc_allocations_cnt = $realloc_allocations_cnt_end - $realloc_allocations_cnt_start;
    $realloc_mem_allocated = $realloc_mem_allocated_end - $realloc_mem_allocated_start;
    fprintf(STDERR, "realloc_allocations_cnt=$realloc_allocations_cnt,realloc_mem_allocated=$realloc_mem_allocated\n");
  } else if ($test_case === "resume_yielded_script_fork_from_oom_handler" ||
             $test_case === "resume_suspended_script_fork_from_oom_handler" ||
             $test_case === "oom_from_fork") {
    $future = fork(fake_query(0.2));
    $ans = wait($future);
    if ($ans !== true) {
      critical_error("fork wait failed, wait(future)=" . var_export($ans, true));
    }
  } else if ($test_case !== "basic" && $test_case !== "timeout_in_oom_handler") {
    critical_error("Unexpected test case $test_case");
  }

  $small_arr = [];
  $small_arr["key"] = "Hello";
  $small_arr["key"] .= "World" . rand(); // test allocations in the end of handler
  unset($small_arr);

  [$allocations_cnt, $mem_allocated] = memory_get_allocations();
  $allocations_cnt -= $allocations_cnt_start;
  $mem_allocated -= $mem_allocated_start;
  $arr_mem_usage = estimate_memory_usage($arr);
  fprintf(STDERR, "allocations_cnt=$allocations_cnt,memory_allocated=$mem_allocated,estimate_memory_usage(arr)=$arr_mem_usage\n");

  if ($test_case === "timeout_in_oom_handler") {
    make_timeout_error();
  }
}

function my_fork(string $test_case) {
  $fork_id = (int)get_running_fork_id();
  fprintf(STDERR, "fork_started_succesfully=%d\n", $fork_id !== 0);
  if ($test_case === "oom_from_fork") {
    make_oom_error();
  } else if ($test_case === "resume_yielded_script_fork_from_oom_handler") {
    sched_yield();
    critical_error("Trying to resume yielded fork started from script in OOM handler");
  } else if ($test_case === "resume_suspended_script_fork_from_oom_handler") {
    $future = fork(fake_query(0.1));
    wait($future);
    critical_error("Trying to resume suspended fork started from script in OOM handler");
  } else {
    critical_error("Unknown test_case=$test_case in fork");
  }
  return null;
}

function main() {
  global $script_allocated_array, $test_case;
  register_shutdown_function(function() {
    critical_error("Shutdown functions shouldn't be called after OOM handler");
  });
  $test_case = (string)$_GET["test_case"];
  $script_allocated_array = [1,2,3,4,5];
  $script_allocated_array[] = rand();
  $ok = register_kphp_on_oom_callback('oom_handler');
  if (!$ok) {
    critical_error("Can't register OOM handler");
  }
  if ($test_case === "oom_from_fork") {
    $future = fork(my_fork($test_case));
    wait($future);
  } else if ($test_case === "resume_yielded_script_fork_from_oom_handler" || $test_case === "resume_suspended_script_fork_from_oom_handler") {
    fork(my_fork($test_case));
    make_oom_error();
  } else {
    make_oom_error();
  }
  critical_error("Must be unreachable - OOM expected\n");
}

main();
