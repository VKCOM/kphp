@ok
<?php
require_once 'kphp_tester_include.php';

/** 
 * @kphp-serializable
 * @kphp-reserved-fields [3]
 **/
class PrimitiveHolder {
    /**
     * @var int
     */
    public static $unused_x = 20;

    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $int_f = 11;

    /**
     * @kphp-serialized-field 2
     * @var float
     */
    public $float_f = 20.123;

    /**
     * @kphp-serialized-field 4
     * @var float
     */
    public $float_NaN_f = NAN;

    /**
     * @kphp-serialized-field 5
     * @var bool
     */
    public $bool_false_f = false;

    /**
     * @kphp-serialized-field 6
     * @var bool
     */
    public $bool_true_f = true;

    /**
     * @kphp-serialized-field 7
     * @var null
     */
    public $null_f = NULL;

    /**
     * @kphp-serialized-field 8
     * @var int
     */
    public $negative_int_f = -12312323;


    /**
     * @param $another PrimitiveHolder
     * @return bool
     */
    public function equal_to($another) {
        var_dump($another->int_f);
        var_dump($another->float_f);
        var_dump($another->float_NaN_f);
        var_dump($another->bool_false_f);
        var_dump($another->bool_true_f);
        var_dump($another->null_f);
        var_dump($another->negative_int_f);

        return $this->int_f === $another->int_f
            && $this->float_f === $another->float_f
            && is_nan($this->float_NaN_f) && is_nan($another->float_NaN_f)
            && $this->bool_false_f === $another->bool_false_f
            && $this->bool_true_f === $another->bool_true_f
            && $this->null_f === $another->null_f
            && $this->negative_int_f === $another->negative_int_f;
    }
}

function run() {
    $b = new PrimitiveHolder();
    $serialized = instance_serialize($b);
    var_dump(base64_encode($serialized));
    $b_new = instance_deserialize($serialized, PrimitiveHolder::class);

    var_dump($b->equal_to($b_new));
}

run();
