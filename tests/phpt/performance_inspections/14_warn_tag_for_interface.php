@kphp_should_warn k2_skip
/variable \$y1 can be reserved with array_reserve functions family out of loop/
/Performance inspection 'array\-reserve' enabled by: TestInterface::interface_function \-> TestInterfaceImpl1::interface_function/
/expression \$x = array_merge\(\$x, <...>\) can be replaced with array_merge_into\(\$x, <...>\)/
/Performance inspection 'array\-merge\-into' enabled by: TestInterface::interface_function \-> TestInterfaceImpl2::interface_function/
<?php

/**
 * @kphp-warn-performance array-reserve array-merge-into
 */
interface TestInterface {
  public function interface_function();
}

class TestInterfaceImpl1 implements TestInterface {
  public function interface_function() {
    $y1 = [];
    for ($i = 0; $i != 1000; ++$i) {
      $y1[] = $i;
    }
    return $y1;
  }
}

class TestInterfaceImpl2 implements TestInterface {
  public function interface_function() {
    $x = [1, 2, 3 ,4];
    if (1) {
      $x = array_merge($x, [1]);
    }
    return $x;
  }
}

function test() {
  /** @var TestInterface $i */
  $i = 1 ? new TestInterfaceImpl1 : new TestInterfaceImpl2;
  $i->interface_function();
}

test();
