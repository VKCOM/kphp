@kphp_should_fail
/Can not store instance with interface inside with instance_cache_store call/
<?php

interface IX {
  public function foo();
}

class X1 implements IX {
  public function foo() {}

  public $x = 1;
}

class Y {
  /** @var IX|false */
  public $ix = false;
}

instance_cache_store("key", new Y);
