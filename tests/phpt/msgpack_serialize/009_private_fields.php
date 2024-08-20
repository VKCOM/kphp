@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

/** 
 * @kphp-serializable
 **/
class A {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    private $y = 10;

    public function equal_to(A $another) : bool {
        return $this->y === $another->y;
    }
}

/** 
 * @kphp-serializable
 **/
class B {
    /**
     * @kphp-serialized-field 1
     * @var A
     */
    protected $a = null;

    public function __construct() { $this->a = new A(); }

    public function equal_to(B $another) : bool {
        return $this->a->equal_to($another->a);
    }
}

function run() {
    $b = new B();
    $serialized = instance_serialize($b);
    var_dump(base64_encode($serialized));
    $b_new = instance_deserialize($serialized, B::class);

    var_dump($b->equal_to($b_new));
#ifndef KPHP
    try {
        $b->y = 20;
    } catch (\Throwable $_) {
        var_dump("OK");
        return;
    }
#endif

    var_dump("OK");
}

run();
