@ok
<?php
require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class IntHolder {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $int_f = 10;
}

/** @kphp-serializable */
class StringHolder {
    /**
     * @kphp-serialized-field 1
     * @var string
     */
    public $string_f = "here";
}

/** @kphp-serializable */
class InstanceHolder {
    /**
     * @kphp-serialized-field 1
     * @var IntHolder
     */
    public $int_holder_f;

    /**
     * @kphp-serialized-field 2
     * @var IntHolder[]
     */
    public $int_holder_arr_f;

    /**
     * @kphp-serialized-field 3
     * @var \tuple((IntHolder[] |   false) [][],StringHolder)  some comment
     */
    public $tuple_holders_int_string_f;

    /**
     * @kphp-serialized-field 4
     * @var IntHolder
     */
    public $int_holder_null_f = null;

    /** @return InstanceHolder */
    public static function get_instance() {
        $res = new InstanceHolder();
        $res->int_holder_f = new IntHolder();
        $res->int_holder_arr_f = [new IntHolder(), $res->int_holder_f];
        $res->tuple_holders_int_string_f = tuple([[[new IntHolder()], false]], new StringHolder());
        if (false) {
            $res->int_holder_null_f = new IntHolder();
        }

        return $res;
    }

    /**
     * @param $another InstanceHolder
     * @return bool
     */
    public function equal_to($another) {
        var_dump($another->int_holder_f->int_f);
        var_dump(count($another->int_holder_arr_f), $another->int_holder_arr_f[0]->int_f);

        /**@var IntHolder*/
        $int_holder_this_from_tuple = $this->tuple_holders_int_string_f[0][0][0][0];
        /**@var IntHolder */
        $another_holder_this_from_tuple = $another->tuple_holders_int_string_f[0][0][0][0];

        var_dump($another_holder_this_from_tuple->int_f, (bool)$another->tuple_holders_int_string_f[0][0][1], $another->tuple_holders_int_string_f[1]->string_f);
        var_dump($another->int_holder_null_f === null);

        return $this->int_holder_f->int_f === $another->int_holder_f->int_f
          && count($this->int_holder_arr_f) === count($another->int_holder_arr_f)
          && $this->int_holder_arr_f[0]->int_f === $another->int_holder_arr_f[0]->int_f
          && $int_holder_this_from_tuple->int_f === $another_holder_this_from_tuple->int_f
          && $this->tuple_holders_int_string_f[0][0][1] === $another->tuple_holders_int_string_f[0][0][1]
          && $this->tuple_holders_int_string_f[1]->string_f === $another->tuple_holders_int_string_f[1]->string_f
          && $this->int_holder_null_f === $another->int_holder_null_f;
    }
}

function run() {
    $instance = InstanceHolder::get_instance();

    $serialized = instance_serialize($instance);
    var_dump(base64_encode($serialized));
    $b_new = instance_deserialize($serialized, InstanceHolder::class);

    var_dump($instance->equal_to($b_new));
}

run();
