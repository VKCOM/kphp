@kphp_should_fail
/\$foo could be null in instance property fetch/
/\$foo->next could be null in instance property fetch/
<?php

class Foo {
  public $prop = 10;
  public ?Foo $next = null;

  public function f() { return "f()"; }
}

function test(?Foo $foo) {
  var_dump($foo->prop);
  var_dump($foo->next->prop);
}

test(new Foo());
test(null);
