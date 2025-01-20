@ok
<?php
require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class NonPrimitivesHolder {
    /**
     * @kphp-serialized-field 2
     * @var ?string
     */
    public $str_f = null;

    /**
     * @param $another NonPrimitivesHolder
     * @return bool
     */
    public function equal_to($another) {
        var_dump($another->str_f);

        return $this->str_f === $another->str_f;
    }
}

function run() {
    $b = new NonPrimitivesHolder();
    $b->str_f = "asdf";
    $serialized = instance_serialize($b);
    var_dump(base64_encode($serialized));

    $b_new = instance_deserialize($serialized, NonPrimitivesHolder::class);
    var_dump($b->equal_to($b_new));
}

run();
