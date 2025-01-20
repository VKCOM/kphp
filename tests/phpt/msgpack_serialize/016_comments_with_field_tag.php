@ok
<?php

require_once 'kphp_tester_include.php';

/** 
 * @kphp-serializable
 **/
class A {
    /**
     * @kphp-serialized-field 1 // comment
     * @var int
     */
    public $y = 10;

    /**
     * @kphp-serialized-field none comment
     * @var int
     */
    public $z = 10;
}

function run() {
    $a = new A();
    $serialized = instance_serialize($a);
    var_dump(base64_encode($serialized));
    $a_new = instance_deserialize($serialized, A::class);
}

run();
