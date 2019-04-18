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

class X2 implements IX {
  public function foo() {}

  public $x = 2;
}

/** @var IX[] */
$xx = [new X1, new X2];
instance_cache_store("key", $xx[0]);
