@ok
<?php

require_once 'polyfills.php';

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

try {
    $serialized = instance_serialize($a);
} catch (Exception $e) {
    var_dump($e->getMessage());
}

