@ok
<?php

trait TraitFoo1 {
  public abstract function foo(int $a);
}

trait TraitFoo2 {
  public abstract function foo(int $a);
}

class C {
  use TraitFoo1;
  use TraitFoo2;

  public function foo(int $a) {}
}

$c = new C();
