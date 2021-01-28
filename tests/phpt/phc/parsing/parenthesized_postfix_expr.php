@ok
<?php

class Foo {
  public $x = 10;
}

function test_arrow() {
  var_dump((new Foo())->x);

  $foo = new Foo();
  var_dump(($foo)->x);
}

function test_call() {
  $fn = function() { return 'ok'; };
  var_dump(($fn)());
}

test_arrow();
test_call();
