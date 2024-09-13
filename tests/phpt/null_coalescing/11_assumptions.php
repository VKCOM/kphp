@ok k2_skip
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
   * @param int $x
   * @return AI
   */
  public function make_a($x): AI {
    return $x == 1 ? new A1 : new A2;
  }
}

function test_null_coalesce_operator(?B $b) {
  $factory = $b ?? null;
  if ($factory) {
    $a1 = null ?? $factory->make_a(1);
    $a1->say_hello();
    $a2 = $factory->make_a(2) ?? $a1 ?? null;
    $a2->say_hello();
  }
}


test_null_coalesce_operator(new B);
test_null_coalesce_operator(null);
