@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @kphp-serializable
 * @kphp-reserved-fields [3]
 **/
class PrimitiveHolder {
    public static int $unused_x = 20;

    /**
     * @kphp-serialized-field 1
     */
    public int $int_f = 11;

    /**
     * @kphp-serialized-field 2
     */
    public float $float_f = 20.123;

    /**
     * @kphp-serialized-field 4
     */
    public float $float_NaN_f = NAN;

    /**
     * @kphp-serialized-field 5
     */
    public bool $bool_false_f = false;

    /**
     * @kphp-serialized-field 6
     */
    public bool $bool_true_f = true;

    /**
     * @kphp-serialized-field 7
     */
    public int $negative_int_f = -12312323;

    /**
     * @kphp-serialized-field 8
     */
    public ?string $s_null_null = NULL;

    /**
     * @kphp-serialized-field 9
     */
    public ?string $s_null_s = 'ss';

    /**
     * @kphp-serialized-field 10
     */
    public ?bool $b_null_null = null;

    /**
     * @kphp-serialized-field 11
     */
    public ?bool $b_null_b = true;

    /**
     * @kphp-serialized-field 12
     */
    public ?int $i_null_null = null;

    /**
     * @kphp-serialized-field 13
     */
    public ?int $i_null_0 = 0;


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
        var_dump($another->negative_int_f);
        var_dump($another->s_null_null);
        var_dump($another->s_null_s);
        var_dump($another->b_null_null);
        var_dump($another->b_null_b);
        var_dump($another->i_null_null);
        var_dump($another->i_null_0);

        return $this->int_f === $another->int_f
            && $this->float_f === $another->float_f
            && is_nan($this->float_NaN_f) && is_nan($another->float_NaN_f)
            && $this->bool_false_f === $another->bool_false_f
            && $this->bool_true_f === $another->bool_true_f
            && $this->negative_int_f === $another->negative_int_f
            && $this->s_null_null === $another->s_null_null
            && $this->s_null_s === $another->s_null_s
            && $this->b_null_null === $another->b_null_null
            && $this->b_null_b === $another->b_null_b
            && $this->i_null_null === $another->i_null_null
            && $this->i_null_0 === $another->i_null_0
            ;
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
