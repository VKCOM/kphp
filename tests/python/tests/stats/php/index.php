<?php

function sleep_for_net_time() {
  sched_yield_sleep(0.01);
  return null;
}

function wait_for_script_time() {
  $t = microtime(true) + 0.01;
  while($t > microtime(true)) {}
}

function run_default() {
  wait_for_script_time();

  $i1 = fork(sleep_for_net_time());
  wait($i1);
}

function get_stats_by_mc() {
  $mc = new McMemcache();
  $mc->addServer("localhost", (int)$_GET["master-port"], true, 1, 10);

  $attempts = 3;
  do {
    $result = $mc->get("stats_full");
    if (is_string($result)) {
      echo $result;
      return;
    }
  } while (--$attempts >= 0);
  critical_error("mc get timeout!");
}

function get_stats_by_rpc() {
  $connection = new_rpc_connection("localhost", (int)$_GET["master-port"]);
  $req = rpc_tl_query_one($connection, ['_' => "engine.stat"]);
  $result = rpc_tl_query_result_one($req);
  echo json_encode($result["result"]);
}

function do_http_worker() {
  switch($_SERVER["PHP_SELF"]) {
    case "/get_stats_by_mc": {
      get_stats_by_mc();
      return;
    }
    case "/get_stats_by_rpc": {
      get_stats_by_rpc();
      return;
    }
    case "/get_webserver_stats": {
      $stats = get_webserver_stats();
      echo "running_workers:".$stats[0]."\n";
      echo "waiting_workers:".$stats[1]."\n";
      echo "ready_for_accept_workers:".$stats[2]."\n";
      echo "total_workers:".$stats[3]."\n";
      return;
    }
  }

  run_default();
  echo "Hello world!";
}

do_http_worker();
