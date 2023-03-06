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
  warning($msg);
}

/** @kphp-required */
function shutdown_after_long_work() {
  // unless the timer resets to 0 before this function is executed, it will not
  // manage to finish successfully
  do_sleep(0.75);
  warning("shutdown function managed to finish");
}

/** @kphp-required */
function shutdown_endless_loop() {
  while (true) {}
}

/** @kphp-required */
function shutdown_with_exit1() {
  warning("running shutdown handler 1");
  exit(0);
}

/** @kphp-required */
function shutdown_with_exit2() {
  warning("running shutdown handler 2");
}

/** @kphp-required */
function shutdown_critical_error() {
  warning("running shutdown handler critical errors");
  critical_error("critical error from shutdown function");
}

/** @kphp-required */
function shutdown_fork_wait() {
  warning("before fork");
  $resp_future = fork(forked_func(42));
  warning("after fork");
  while (!wait_concurrently($resp_future)) {}
  warning("after wait");
}

function forked_func(int $i): int {
  warning("before yield");
  sched_yield_sleep(0.5); // wait net
  warning("after yield");
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

interface Iface {
  public function f();
}

class Impl implements Iface {
  public function f() { var_dump(0); }
}

function do_null_interface_method_call(Iface $iface) {
  $iface->f();
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
      case "sleep":
        do_sleep((float)$action["duration"]);
        break;
      case "register_shutdown_function":
        do_register_shutdown_function((string)$action["msg"]);
        break;
      case "null_interface_method_call":
        $impl = new Impl();
        $impl = null;
        do_null_interface_method_call($impl);
        break;
      default:
        echo "unknown operation";
        return;
    }
  }
  echo "ok";
}

main();
