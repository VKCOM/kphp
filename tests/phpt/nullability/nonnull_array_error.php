@ok
<?php

class Foo {
  public $prop = 10;

  public function f() { return "f()"; }
}

function test() {
  /** @var (?Foo)[] */
  $array = [new Foo()];
  $array[] = new Foo();
  $array[] = null;
  $x = $array[0];
  if ($x !== null) {
    var_dump($x->prop);
    var_dump($x->f());
  }
}

test();
