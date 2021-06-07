@ok
<?php

class Foo {
  public $prop = 10;

  public function f() { return "f()"; }
}

function test() {
  /** @var Foo[] */
  $array = [new Foo()];
  $array[] = new Foo();
  var_dump($array[0]->prop);
  var_dump($array[0]->f());
}

test();
