@ok
KPHP_ERROR_ON_WARNINGS=1
<?php

function test_replace_comparison_on_isset_for_var() {
  $x = [1, 2, 3];
  if ($x["xxx"] !== null) {
    echo "hello var!\n";
  }
}

function test_replace_comparison_on_isset_for_instance_var() {
  class A {
    public $x = [1, 2, 3];
  }

  $a = new A;
  if ($a->x["xxx"] !== null) {
    echo "hello instance var\n";
  }
}

function test_replace_comparison_on_isset_for_static() {
  class C {
    static $x = [1, 2, 3];
  }

  if (C::$x["xxx"] !== null) {
    echo "hello static\n";
  }
}

function test_replace_comparison_on_isset_for_const_int() {
  class D {
    const X = [1, 2, 3];
  }

  if (D::X['test'] !== null) {
    echo "hello constant int\n";
  }
}

function test_replace_comparison_on_isset_for_const_string() {
 class E {
   const X = ["1", "2", "3"];
 }

 if (E::X['test'] !== null) {
   echo "hello constant string\n";
 }
}

test_replace_comparison_on_isset_for_var();
test_replace_comparison_on_isset_for_instance_var();
test_replace_comparison_on_isset_for_static();
test_replace_comparison_on_isset_for_const_int();
test_replace_comparison_on_isset_for_const_string();
