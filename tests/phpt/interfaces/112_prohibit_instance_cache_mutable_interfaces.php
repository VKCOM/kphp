@kphp_should_fail
<?php

interface IX { public function dump_me(); }
class X implements IX {
    public $x = 10;
    public function dump_me() { var_dump($this->x); }
}

class Y implements IX {
    public function dump_me() { var_dump(10); }
}

/** @var IX $y */
$y = new X();
$y = new Y();

var_dump(instance_cache_store("key_x1", $y));

$x = instance_cache_fetch(Y::class, "key_x1");
$x->dump_me();
