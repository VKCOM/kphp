@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

class ClassB {
  public function func_b(ClassB $x) {
    $f = function () {
      while(0);

      echo "\n---------------------------------\n";
      var_dump(count(kphp_backtrace()) >= 5);
      echo "\n---------------------------------\n";
      var_dump(count(kphp_backtrace(false)) >= 5);
    };
    while(0);
    $f();
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
