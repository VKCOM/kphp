@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function ret_bool(int $line, bool $res) : bool {
  echo "call ret_bool:$line\n";
  sched_yield();
  return $res;
}

function ret_string(int $line, string $res) : string {
  echo "call ret_string:$line\n";
  sched_yield();
  return $res;
}

function ret_array(int $line, array $res) : array {
  echo "call ret_array:$line\n";
  sched_yield();
  return $res;
}

function ret_int(int $line, int $res) : int {
  echo "call ret_int:$line\n";
  sched_yield();
  return $res;
}

function test_ternary_true_case() {
  $s = 1 ? ret_string(__LINE__, "aaaaaaa") : "bbbbbb";
  var_dump($s);

  $s = 0 ? ret_string(__LINE__, "ccccccc") : "dddddd";
  var_dump($s);
  return null;
}

function test_ternary_false_case() {
  $s = 1 ? "eeeeee" : ret_string(__LINE__, "fffffff");
  var_dump($s);

  $s = 0 ? "gggggg" : ret_string(__LINE__, "hhhhhhhh");
  var_dump($s);
  return null;
}

function test_ternary_true_false_case() {
  $s = 1 ? ret_string(__LINE__, "++++++++++")
         : ret_string(__LINE__, "----------");
  var_dump($s);

  $s = 0 ? ret_string(__LINE__, "****************")
         : ret_string(__LINE__, "//////////");
  var_dump($s);
  return null;
}

function test_ternary_condition() {
  $s = ret_bool(__LINE__, true) ? "11111" : "22222";
  var_dump($s);

  $s = ret_bool(__LINE__, false) ? "333333" : "444444";
  var_dump($s);

  $s = !ret_bool(__LINE__, true) ? "aaaaaa" : "bbbbbb";
  var_dump($s);

  $s = !ret_bool(__LINE__, false) ? "zzzzzz" : "yyyyyyy";
  var_dump($s);

  $s = ret_string(__LINE__, "1") ? "555555" : "666666";
  var_dump($s);

  $s = ret_string(__LINE__, "0") ? "77777" : "88888888";
  var_dump($s);
  return null;
}

function test_ternary_condition_true_case() {
  $s = ret_bool(__LINE__, true) ? ret_string(__LINE__, "aaaaaaa") : "bbbbbb";
  var_dump($s);

  $s = ret_bool(__LINE__, false) ? ret_string(__LINE__, "ccccccc") : "dddddd";
  var_dump($s);

  $s = ret_string(__LINE__, "ccccccc") ?: "dddddd";
  var_dump($s);

  $s = ret_string(__LINE__, "0") ?: "dddddd";
  var_dump($s);
  return null;
}


function test_ternary_condition_false_case() {
  $s = ret_bool(__LINE__, true) ? "xxxxxxx" : ret_string(__LINE__, "yyyyyyyyy");
  var_dump($s);

  $s = ret_bool(__LINE__, true) ? "1111111" : ret_string(__LINE__, "2222222");
  var_dump($s);
  return null;
}

function test_ternary_condition_true_false_case() {
  $s = ret_bool(__LINE__, true) ? ret_string(__LINE__, "aaaaaaaaa")
                                : ret_string(__LINE__, "bbbbbbbb");
  var_dump($s);

  $s = ret_bool(__LINE__, true) ? ret_string(__LINE__, "111111111")
                                : ret_string(__LINE__, "22222222");
  var_dump($s);

  $s = ret_string(__LINE__, "ccccccc")
        ?: ret_string(__LINE__, "dddddddd");
  var_dump($s);

  $s = ret_string(__LINE__, "0")
        ?: ret_string(__LINE__, "ffffffff");
  var_dump($s);

  $b = !ret_bool(__LINE__, true) ? !ret_string(__LINE__, "0")
                                 : !ret_string(__LINE__, "1");
  var_dump($b);

  $i = ret_bool(__LINE__, true) ? (int)ret_string(__LINE__, "10")
                                 : (int)ret_string(__LINE__, "20");
  var_dump($i);
  return null;
}

function test_array_insertion_operation() {
  $a = [];
  $a[] = ret_bool(__LINE__, true) ? ret_string(__LINE__, "aaaaaaaaa")
                                  : ret_string(__LINE__, "bbbbbbbb");
  $a["xxx"] = ret_bool(__LINE__, true) ? ret_string(__LINE__, "111111111")
                                       : ret_string(__LINE__, "22222222");
  var_dump($a);
  return null;
}

function test_in_if() {
  if (ret_bool(__LINE__, true) ? !ret_string(__LINE__, "1")
                               : ret_string(__LINE__, "0")) {
    echo "if body\n";
  } else {
    echo "else body\n";
  }
  return null;
}


function test_in_return_impl() {
  return ret_bool(__LINE__, true) ? ret_string(__LINE__, "1111111111")
                                  : ret_string(__LINE__, "0000000000000");
}

function test_in_return() {
  $x = test_in_return_impl();
  var_dump($x);
  return null;
}

function test_in_list() {
  [$x, $y, $z] = ret_bool(__LINE__, true) ? ret_array(__LINE__, ["xxx", "yyy", "zzz"])
                                          : ret_array(__LINE__, ["aaa", "bbb", "ccc"]);
  var_dump($x);
  var_dump($y);
  var_dump($z);
  return null;
}

function test_with_logical_op() {
  $a = (ret_bool(__LINE__, true) ||
        ret_bool(__LINE__, false))
        ? ret_array(__LINE__, ["xxx", "yyy", "zzz"])
        : ret_array(__LINE__, ["aaa", "bbb", "ccc"]);
  var_dump($a);

  $a = (ret_bool(__LINE__, true) &&
        ret_bool(__LINE__, false))
        ? ret_array(__LINE__, ["111", "222", "333"])
        : ret_array(__LINE__, ["444", "555", "666"]);
  var_dump($a);
  return null;
}

function test_other_op_set() {
  $a = ret_bool(__LINE__, true) ? ret_int(__LINE__, 1)
                                : ret_int(__LINE__, 42);
  var_dump($a);

  $a += ret_bool(__LINE__, true) ? ret_int(__LINE__, 412)
                                 : ret_int(__LINE__, 12);
  var_dump($a);

  $a -= ret_bool(__LINE__, false) ? ret_int(__LINE__, 200)
                                  : ret_int(__LINE__, 400);
  var_dump($a);

  $a *= ret_int(__LINE__, 3)
        ?: ret_int(__LINE__, 2);
  var_dump($a);

  $a &= ret_int(__LINE__, 0)
        ?: ret_int(__LINE__, 15);
  var_dump($a);
  return null;
}

wait(fork(test_ternary_true_case()));
wait(fork(test_ternary_false_case()));
wait(fork(test_ternary_true_false_case()));
wait(fork(test_ternary_condition()));
wait(fork(test_ternary_condition_true_case()));
wait(fork(test_ternary_condition_false_case()));
wait(fork(test_ternary_condition_true_false_case()));
wait(fork(test_array_insertion_operation()));
wait(fork(test_in_if()));
wait(fork(test_in_return()));
wait(fork(test_in_list()));
wait(fork(test_with_logical_op()));
wait(fork(test_other_op_set()));
