<?php

function do_sigsegv() {
  raise_sigsegv();
}

function do_exception(string $message, int $code) {
  throw new Exception($message, $code);
}

function do_warning(string $message) {
  warning($message);
}

function do_set_context(array $tags, array $extra_info, string $env) {
  kphp_set_context_on_error($tags, $extra_info, $env);
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

function main() {
  foreach (json_decode(file_get_contents('php://input')) as $action) {
    switch ($action["op"]) {
      case "sigsegv":
        do_sigsegv();
        break;
      case "exception":
        do_exception((string)$action["msg"], (int)$action["code"]);
        break;
      case "warning":
        do_warning((string)$action["msg"]);
        break;
      case "set_context":
        do_set_context((array)$action["tags"], (array)$action["extra_info"], (string)$action["env"]);
        break;
      case "stack_overflow":
        do_stack_overflow(1);
        break;
      default:
        echo "unknown operation";
        return;
    }
  }
  echo "ok";
}

main();
