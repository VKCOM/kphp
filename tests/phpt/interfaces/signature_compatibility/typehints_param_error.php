@kphp_should_fail
/Impl::f\(\) should not have a \$x param type hint, it would be incompatible with Base::f\(\)/
<?php

// Error: no param type hint in a base method, but it's specified in derived method

abstract class Base {
  public abstract function f($x);
}

class Impl extends Base {
  public function f(int $x) {}
}
