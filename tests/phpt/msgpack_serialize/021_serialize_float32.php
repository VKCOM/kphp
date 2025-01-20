@ok
<?php

require_once 'kphp_tester_include.php';

/** 
 * @kphp-serializable
 **/
class PrimitiveHolder {
    /**
     * @kphp-serialized-field 3
     * @var int
     */
    public $int_f = 536;

    /**
     * @kphp-serialized-field 2
     * @kphp-serialized-float32
     * @var float
     */
    public $float32_f = 20.123;

    /**
     * @kphp-serialized-field 1
     * @var float
     */
    public $float_f = 20.123;


    public function equal_to(PrimitiveHolder $another): bool {
        var_dump($another->float_f);
        var_dump($another->float32_f);
        var_dump($another->int_f);

        return $this->int_f === $another->int_f
            && $this->float_f === $another->float_f
            && ($this->float32_f - $another->float32_f) < 0.01;
    }
}

function run() {
    $b = new PrimitiveHolder();
    $serialized = instance_serialize($b);
    var_dump(base64_encode($serialized));
    var_dump(strlen($serialized));
    $b_new = instance_deserialize($serialized, PrimitiveHolder::class);

    var_dump($b->equal_to($b_new));
}

run();
