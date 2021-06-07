@ok
<?php

class Foo {
  public $prop = 10;

  public function f() { return "f()"; }
}

function test(Foo $foo) {
  var_dump($foo->prop);
  var_dump($foo->f());
}

test(new Foo());
