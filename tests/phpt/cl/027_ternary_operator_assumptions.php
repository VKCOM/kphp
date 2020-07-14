@ok
<?php

interface AI {
  public function say_hello();
}

class A1 implements AI {
  public function say_hello() {
    echo "hello A1\n";
  }
}

class A2 implements AI {
  public function say_hello() {
    echo "hello A2\n";
  }
}

class B {
  /**
   * @kphp-infer
   * @param int $x
   * @return AI
   */
  public function make_a($x): AI {
    return $x == 1 ? new A1 : new A2;
  }
}

function test_ternary_operator(?B $b, bool $f) {
  $factory = $b ? $b : null;
  if ($factory) {
    $a = $f ? $factory->make_a(1) : $factory->make_a(2);
    $a->say_hello();
  }
}

function test_ternary_operator_short(?B $b) {
  $factory = $b ?: null;
  if ($factory) {
    $a1 = $factory->make_a(1);
    $a = $a1 ?: $factory->make_a(2);
    $a->say_hello();
  }
}

test_ternary_operator(new B, true);
test_ternary_operator(new B, false);
test_ternary_operator(null, true);

test_ternary_operator_short(new B);
test_ternary_operator_short(null);
