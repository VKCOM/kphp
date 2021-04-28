@ok
<?php

class Foo {}

abstract class Base {
  /** @return Foo[] */
  public abstract function f();
}

class Impl1 extends Base {
  /** @return Foo[] */
  public function f(): array { return [new Foo()]; }
}

class Impl2 extends Base {
  /** @return Foo[] */
  public function f() { return [new Foo()]; }
}

/** @var Base[] $list */
$list = [new Impl1(), new Impl2()];
