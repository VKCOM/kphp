@kphp_should_fail k2_skip
/Can not store instance of non-serializable class Y with instance_cache_store call/
<?php

require_once 'kphp_tester_include.php';

interface IX {
  public function foo();
}

/** @kphp-immutable-class */
class X1 implements IX {
  public function foo() {}

  public $x = 1;
}


class X12 extends X1 {

}

/** @kphp-immutable-class */
class Y {
  /** @var IX|null */
  public $ix = null;
}

instance_cache_store("key", new Y());
