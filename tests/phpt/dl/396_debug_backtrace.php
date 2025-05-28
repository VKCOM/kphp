@ok
KPHP_EXTRA_CXXFLAGS=-O0
<?php

require_once 'kphp_tester_include.php';

function my_array_find($arr, callable $callback) {
    foreach($arr as $element) {
        if ($callback($element)) {
            return $element;
        }
    }
    return null;
}

function capture_backtrace() {
#ifndef K2
    $trace = kphp_backtrace();
    $backtrace = [];
    foreach($trace as $function) {
        $backtrace[] = ["function" => $function];
    }
    return $backtrace;
#endif
    return debug_backtrace();
}

function check_backtrace($backtrace) {
    var_dump(my_array_find($backtrace, function(array $value) {
        return stristr($value["function"], "func_b") != false;
    }) != null);

    var_dump(my_array_find($backtrace, function(array $value) {
        return stristr($value["function"], "func_a") != false;
    }) != null);

    var_dump(my_array_find($backtrace, function(array $value) {
        return stristr($value["function"], "func_3") != false;
    }) != null);

    var_dump(my_array_find($backtrace, function(array $value) {
        return stristr($value["function"], "func_2") != false;
    }) != null);

    var_dump(my_array_find($backtrace, function(array $value) {
        return stristr($value["function"], "func_1") != false;
    }) != null);
}

class ClassB {
  public function func_b(ClassB $x) {
    while(0);
    sched_yield();
    $backtrace = capture_backtrace();
    check_backtrace($backtrace);
  }
}

class ClassA extends ClassB  {
  public function func_a(int $x) {
    while(0);
    self::func_b(new ClassB);
  }
}

function fun3(?array $x) {
  while(0);
  (new ClassA)->func_a($x ? count($x) : -1);
}

function fun2(array $x) {
  while(0);
  fun3(false ? $x : null);
}

function fun1() {
  while(0);
  fun2([1, 2, 3, 4, 5, 6, 7]);
}

fun1();
