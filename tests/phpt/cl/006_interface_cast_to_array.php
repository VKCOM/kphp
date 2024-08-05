@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

interface IFoo {}

class Foo implements IFoo {
  public $x = "foo";
}

class Bar implements IFoo {
  public $y = "bar";
}

class Baz implements IFoo {
  public $y = "baz";
}

class Gaz implements IFoo {
  /** @var IFoo[] */
  public $a = [];

  /** @var (tuple(IFoo,string)|false)[] */
  public $at = [];
}

function simple_case() {
  /** @var IFoo */
  $foo = new Foo();
  $foo = new Bar();

  var_dump(to_array_debug($foo));
}

function complex_case() {
  $g = new Gaz;
  $g->a[] = new Bar;
  $g->a[] = new Foo;
  $g->a[] = new Gaz;
  $g->at[] = tuple(new Baz, "new Baz");
  $g->at[] = false;
  $g->at[] = tuple(new Gaz, "new Gaz");

  var_dump(to_array_debug($g));
}

simple_case();
complex_case();
