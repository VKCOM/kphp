@kphp_should_fail
/pass \?Foo to argument \$this of Foo::f/
/declared as @param Foo/
<?php

class Foo {
  public $prop = 10;
  public ?Foo $next = null;

  public function f() { return "f()"; }
}

function test(?Foo $foo) {
  var_dump($foo->next->f());
}

test(new Foo());
test(null);
