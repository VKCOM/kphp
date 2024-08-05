@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function ret_int_or_null(int $line, ?int $res) : ?int {
  echo "call ret_float_or_null:$line\n";
  sched_yield();
  return $res;
}

function ret_string_or_null(int $line, ?string $res) : ?string {
  echo "call ret_string_or_null:$line\n";
  sched_yield();
  return $res;
}

function ret_array_or_null(int $line, ?array $res) : ?array {
  echo "call ret_array_or_null:$line\n";
  sched_yield();
  return $res;
}

function test_arithmetic_set() {
  /** @var int $x */
  $x = 123;

  $x += ret_int_or_null(__LINE__, null);
  $x += ret_int_or_null(__LINE__, null) ?? 4;
  $x += ret_int_or_null(__LINE__, null) ?: 7;

  $x += ret_int_or_null(__LINE__, 5);
  $x -= ret_int_or_null(__LINE__, 2);
  $x *= ret_int_or_null(__LINE__, 3);
  $x %= ret_int_or_null(__LINE__, 15);

  var_dump($x);

  return null;
}

function test_array_set() {
  /** @var (string|null)[] $x */
  $x = ["abc"];

  $x[] = ret_string_or_null(__LINE__, null);
  $x[] = ret_string_or_null(__LINE__, null) ?? "aa11";
  $x[] = ret_string_or_null(__LINE__, null) ?: "aa22";

  $x[] = ret_string_or_null(__LINE__, "bb11");
  $x[] = ret_string_or_null(__LINE__, "bb22") ?? "bb33";
  $x[] = ret_string_or_null(__LINE__, "bb44") ?: "bb55";

  $x["cc00"] = ret_string_or_null(__LINE__, null);
  $x["cc11"] = ret_string_or_null(__LINE__, null) ?? "cc22";
  $x["cc33"] = ret_string_or_null(__LINE__, null) ?: "cc44";

  $x["cc55"] = ret_string_or_null(__LINE__, "cc66");
  $x["cc77"] = ret_string_or_null(__LINE__, "cc88") ?? "cc99";
  $x["cc1010"] = ret_string_or_null(__LINE__, "cc1111") ?: "cc1212";

  $x += ret_array_or_null(__LINE__, null) ?? ["dd11"];
  $x += ret_array_or_null(__LINE__, null) ?: ["dd22"];

  $x += ret_array_or_null(__LINE__, ["ee11", "ee22"]);
  $x += ret_array_or_null(__LINE__, ["ee33", "ee44"]) ?? ["ee55"];
  $x += ret_array_or_null(__LINE__, ["ee66", "ee77"]) ?: ["ee88"];

  var_dump($x);

  return null;
}

wait(fork(test_arithmetic_set()));
wait(fork(test_array_set()));
