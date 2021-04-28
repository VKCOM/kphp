@kphp_should_fail
/Impl::f\(\) should provide a return type hint compatible with Base::f\(\)/
<?php

// Error: return type hint in a base method, but not in derived method

class Foo {}

abstract class Base {
  /** @return Foo[] */
  public abstract function f(): array;
}

class Impl extends Base {
  /** @return Foo[] */
  public function f() { return [new Foo()]; }
}

/** @var Base[] $list */
$list = [new Impl()];
