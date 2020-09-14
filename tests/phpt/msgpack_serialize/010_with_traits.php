@ok
<?php

require_once 'kphp_tester_include.php';

trait A {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    private $y = 10;
}

/** 
 * @kphp-serializable
 **/
class B {
    use A;

    /**
     * @kphp-serialized-field 2
     * @var float
     */
    protected $b = 2.78;

    public function equal_to(B $another) : bool {
        return $this->b === $another->b
            && $this->y === $another->y;
    }
}

function run() {
    $b = new B();
    $serialized = instance_serialize($b);
    var_dump(base64_encode($serialized));
    $b_new = instance_deserialize($serialized, B::class);
    var_dump($b->equal_to($b_new));
}

run();
