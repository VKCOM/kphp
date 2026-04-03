<?php

function test_posix_getpid() {
  echo json_encode(array(
    "pid" => posix_getpid()
  ));
}

function test_posix_getuid() {
  echo json_encode(array(
    "uid" => posix_getuid()
  ));
}

function test_posix_getpwuid() {
  echo json_encode(posix_getpwuid(posix_getuid()));
}

function main() {
  switch ($_SERVER["PHP_SELF"]) {
    case "/test_posix_getpid": {
        test_posix_getpid();
        return;
    }
    case "/test_posix_getuid": {
      test_posix_getuid();
      return;
    }
    case "/test_posix_getpwuid": {
      test_posix_getpwuid();
      return;
    }
  }

  critical_error("unknown test");
}

main();
