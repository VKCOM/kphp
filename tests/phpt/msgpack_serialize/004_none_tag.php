@ok
<?php
require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class PrimitiveHolder {
    /**
     * @kphp-serialized-field none
     * @var int
     */
    public $int_f = 11;

    /**
     * @kphp-serialized-field none
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
    $b->bool_true_f = true;
    $serialized = instance_serialize($b);
    var_dump(base64_encode($serialized));

    $b_new = instance_deserialize($serialized, PrimitiveHolder::class);
    var_dump($b->equal_to($b_new));
}

run();
