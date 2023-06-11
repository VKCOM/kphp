<?php

class ServerException extends Exception {}

function may_throw(bool $cond, string $msg) {
  if ($cond) {
    throw new Exception($msg);
  }
}

/** @kphp-required */
function shutdown_exception_warning() {
  $msg = "running shutdown handler";
  try {
    // make KPHP generate exception-aware call that will check the
    // CurException variable; it should be empty even if it had some
    // exceptions before shutdown functions were executed
    may_throw(false, "exception from shutdown handler");
  } catch (Throwable $e) {
    $msg = "unexpected exception in shutdown handler";
  }
  fprintf(STDERR, $msg . "\n");
}

/** @kphp-required */
function shutdown_after_long_work() {
  // unless the timer resets to 0 before this function is executed, it will not
  // manage to finish successfully
  do_sleep(0.75);
  fprintf(STDERR, "shutdown function managed to finish\n");
}

/** @kphp-required */
function shutdown_endless_loop() {
  while (true) {}
}

/** @kphp-required */
function shutdown_with_exit1() {
  fprintf(STDERR, "running shutdown handler 1\n");
  exit(0);
}

/** @kphp-required */
function shutdown_with_exit2() {
  fprintf(STDERR, "running shutdown handler 2\n");
}

/** @kphp-required */
function shutdown_critical_error() {
  fprintf(STDERR, "running shutdown handler critical errors\n");
  critical_error("critical error from shutdown function");
}

/** @kphp-required */
function shutdown_fork_wait() {
  fprintf(STDERR, "shutdown_fork_wait(): running_fork_id=%d\n", (int)get_running_fork_id());
  fprintf(STDERR, "before fork\n");
  $resp_future = fork(forked_func(42));
  fprintf(STDERR, "after fork\n");
  while (!wait_concurrently($resp_future)) {}
  fprintf(STDERR, "after wait\n");
}

function forked_func(int $i, float $duration = 0.5): int {
  fprintf(STDERR, "before yield\n");
  sched_yield_sleep($duration); // wait net
  fprintf(STDERR, "after yield\n");
  return $i;
}

function do_sleep(float $seconds) {
  usleep((int)(1000000 * $seconds));
}

function do_register_shutdown_function(string $fn) {
  switch ($fn) {
    case "shutdown_exception_warning":
      register_shutdown_function("shutdown_exception_warning");
      break;
    case "shutdown_after_long_work":
      register_shutdown_function("shutdown_after_long_work");
      break;
    case "shutdown_endless_loop":
      register_shutdown_function("shutdown_endless_loop");
      break;
    case "shutdown_with_exit":
      register_shutdown_function("shutdown_with_exit1");
      register_shutdown_function("shutdown_with_exit2");
      break;
    case "shutdown_critical_error":
      register_shutdown_function("shutdown_critical_error");
      break;
    case "shutdown_fork_wait":
      register_shutdown_function("shutdown_fork_wait");
      break;
  }
}

function do_sigsegv() {
  raise_sigsegv();
}

function do_exception(string $message, int $code) {
  throw new ServerException($message, $code);
}

function do_stack_overflow(int $x): int {
  $z = 10;
  if ($x) {
    $y = do_stack_overflow($x + 1);
    $z = $y + 1;
  }
  $z += do_stack_overflow($x + $z);
  return $z;
}

function do_long_work(int $duration) {
  $s = time();
  while(time() - $s <= $duration) {
  }
}

function long_resumable(int $duration): bool {
  if (false) {   // make this function reachable from forks to make kphp generate resumable specific code
    fork(long_resumable(-1));
    sched_yield();
  }
  do_long_work($duration);
  return true;
}

function send_rpc(int $master_port, float $duration, bool $expect_resume = true) {
  $c = new_rpc_connection("127.0.0.1", $master_port, 0, 100, 100);
  $req_id = rpc_tl_query_one($c, ["_" => "engine.sleep", "time_ms" => (int)($duration * 1000)]);
  $succ = ($req_id > 0);
  fprintf(STDERR, "RPC request sent successfully = $succ\n");
  $resp = rpc_tl_query_result_one($req_id);
  if (!$expect_resume) {
    critical_error("This fork mustn't be resumed");
  }
  fprintf(STDERR, "Got RPC answer = " . var_export($resp, true) . "\n");
  return $resp["result"];
}

function main() {
  foreach (json_decode(file_get_contents('php://input')) as $action) {
    switch ($action["op"]) {
      case "sigsegv":
        do_sigsegv();
        break;
      case "exception":
        do_exception((string)$action["msg"], (int)$action["code"]);
        break;
      case "stack_overflow":
        do_stack_overflow(1);
        break;
      case "long_work":
        do_long_work((int)$action["duration"]);
        break;
      case "resumable_long_work":
        long_resumable((int)$action["duration"]);
        break;
      case "fork_wait_resumable_long_work":
        $f = fork(long_resumable((int)$action["duration"]));
        wait($f);
        break;
      case "sleep":
        do_sleep((float)$action["duration"]);
        break;
      case "fork_send_rpc_without_wait":
        $master_port = (int)$action["master_port"];
        $duration = (float)$action["duration"];
        fork(send_rpc($master_port, $duration, false));
        rpc_flush();
        do_sleep(0.001);
        rpc_flush();
        do_sleep(0.001);
        rpc_flush();
        break;
      case "send_long_rpc":
        $master_port = (int)$action["master_port"];
        $duration = (float)$action["duration"];
        send_rpc($master_port, $duration);
        break;
      case "register_shutdown_function":
        do_register_shutdown_function((string)$action["msg"]);
        break;
      default:
        echo "unknown operation";
        return;
    }
  }
  echo "ok";
}

main();
