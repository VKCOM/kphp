@ok
<?php

require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class A {
    /**
     * @kphp-serialized-field 1
     * @var B
     */
    public $b;
}

/** @kphp-serializable */
class B {
    /**
     * @kphp-serialized-field 1
     * @var A
     */
    public $a;
}

$a = new A();
$a->b = new B();
$a->b->a = $a;

$serialized = instance_serialize($a);
var_dump(is_null($serialized));

