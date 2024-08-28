@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class PrimitiveHolder {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $int_f = 11;

    /**
     * @kphp-serialized-field 5
     * @var bool
     */
    public $bool_true_f = false;

    /**
     * @param $another PrimitiveHolder
     * @return bool
     */
    public function equal_to($another) {
        var_dump($another->int_f);
        var_dump($another->bool_true_f);

        return $this->int_f === $another->int_f
            && $this->bool_true_f === $another->bool_true_f;
    }
}

function run() {
    $b = new PrimitiveHolder();
    $serialized = instance_serialize($b);
    var_dump(base64_encode($serialized));

    $tmp = substr($serialized, 1, strlen($serialized));
    $res = instance_deserialize($tmp, PrimitiveHolder::class);
    var_dump(is_null($res));

    // causes false positive stack-overflow in asan mode with gcc < 7
    // try {
    //     $tmp = substr($serialized, 0, -1);
    //     instance_deserialize($tmp, PrimitiveHolder::class);
    // } catch (Exception $e) {
    //     var_dump($e->getMessage());
    // }
}

run();
