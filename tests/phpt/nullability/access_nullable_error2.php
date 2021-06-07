@kphp_should_fail
/op_conv_drop_null->next could be null in instance property fetch/
<?php

class Foo {
  public $prop = 10;
  public ?Foo $next = null;

  public function f() { return "f()"; }
}

function test(?Foo $foo) {
  if ($foo !== null) {
    var_dump($foo->next->prop);
  }
}

test(new Foo());
test(null);
