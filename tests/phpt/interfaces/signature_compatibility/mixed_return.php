@ok
<?php

// TODO: replace mixed phpdoc comment with a type hint in PHP8

abstract class A {
  /** @return mixed */
  public abstract function f();
}

class B extends A {
  public function f(): int {}
}

$b = new B();
