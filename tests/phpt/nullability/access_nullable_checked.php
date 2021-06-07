@ok
<?php

class Foo {
  public $prop = 10;
  /** @var ?Foo */
  public $next = null;

  public function f() { return "f()"; }
}

function test(?Foo $foo) {
  if ($foo !== null) {
    var_dump($foo->prop);
    var_dump($foo->f());
  }
}

test(new Foo());
test(null);
