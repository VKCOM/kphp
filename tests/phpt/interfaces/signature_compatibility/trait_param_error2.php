@kphp_should_fail
/pass int to argument \$x of C::foo/
/declared as @param string/
<?php

// PHP is OK with this code *unless* strict_types=1 are used;
// in that case, we'll get an error when passing 10 as a string.
// We forbid such code since it's not a useful behavior.

trait TraitFoo1 {
  public abstract function foo(int $a);

  public function test1() {
    $this->foo(10);
  }
}

trait TraitFoo2 {
  public abstract function foo(int $a);

  public function test2() {
    $this->foo(10);
  }
}

class C {
  use TraitFoo1;
  use TraitFoo2;

  public function foo(string $x) {
    var_dump($x);
  }
}

$c = new C();
$c->test1();
$c->test2();
