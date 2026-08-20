@kphp_should_fail
/Field ix of immutable class Y has mutable derived X12/
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
