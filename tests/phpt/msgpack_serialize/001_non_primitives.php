@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class NonPrimitivesHolder {
    /**
     * @kphp-serialized-field 2
     * @var string
     */
    public $str_f = "asdf";

    /**
     * @kphp-serialized-field 120
     * @var int[]
     */
    public $arr_f = [2, 2, 3];

    /**
     * @kphp-serialized-field 10
     * @var int[]
     */
    public $map_f = ["asdf" => 2, 10 => 2, "zxcv" => 3];

    /**
     * @kphp-serialized-field 90
     * @var mixed
     */
    public $mixed_f = true ? "asdf" : 123;

    /**
     * @kphp-serialized-field 3
     * @var \tuple(string,int)
     */
    public $tuple_string_int_f;

    /**
     * @kphp-serialized-field 34
     * @var int|false
     */
    public $or_int_f = true ? 10 : false;

    /**
     * @kphp-serialized-field 31
     * @var int|false
     */
    public $or_int_false_f = false ? 10 : false;

    /**
     * @kphp-serialized-field 39
     * @var string|null
     */
    public $or_null_string_f = true ? "zxcv" : null;

    /**
     * @kphp-serialized-field 40
     * @var (int|false)[]
     */
    public $or_int_false_arr_f = [false ? 10 : false, 10];

    /**
     * @param $another NonPrimitivesHolder
     * @return bool
     */
    public function equal_to($another) {
        var_dump($another->str_f);
        var_dump($another->arr_f);
        var_dump($another->map_f);
        var_dump($another->mixed_f);
        var_dump($another->tuple_string_int_f[0], $another->tuple_string_int_f[1]);
        var_dump($another->or_int_f);
        var_dump($another->or_int_false_f);
        var_dump($another->or_null_string_f);
        var_dump($another->or_int_false_arr_f);

        return $this->str_f === $another->str_f
            && $this->arr_f === $another->arr_f
            && $this->map_f === $another->map_f
            && $this->mixed_f === $another->mixed_f
            && $this->tuple_string_int_f === $another->tuple_string_int_f
            && $this->or_int_f === $another->or_int_f
            && $this->or_int_false_f === $another->or_int_false_f
            && $this->or_null_string_f === $another->or_null_string_f
            && $this->or_int_false_arr_f === $another->or_int_false_arr_f;
    }
}

function run() {
    $b = new NonPrimitivesHolder();
    $b->tuple_string_int_f = tuple("Hasdf", 123);
    $serialized = instance_serialize($b);
    var_dump(base64_encode($serialized));

    $b_new = instance_deserialize($serialized, NonPrimitivesHolder::class);
    var_dump($b->equal_to($b_new));
}

run();
