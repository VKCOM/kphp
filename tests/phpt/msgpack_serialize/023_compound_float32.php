@ok
<?php

require_once 'kphp_tester_include.php';

/** @kphp-serializable **/
class FloatHolder {
    /**
     * @kphp-serialized-field 1
     * @kphp-serialized-float32
     * @var float[]
     */
    public $float32_f = [20.123, .536, .536];

    /**
     * @kphp-serialized-field 2
     * @var tuple(float, int)
     */
    public $tup;

    public function __construct() {
        $this->tup = tuple(.536, 20);
    }
}

/** @kphp-serializable **/
class A {
    /**
     * @kphp-serialized-field 1
     * @kphp-serialized-float32
     * @var FloatHolder
     */
    public $float_f = null;

    public function __construct() {
        $this->float_f = new FloatHolder();
    }
}

$a = new A();
$serialized = instance_serialize($a);
var_dump(base64_encode($serialized));
var_dump(strlen($serialized));

$h = new FloatHolder();
$serialized = instance_serialize($h);
var_dump(base64_encode($serialized));
var_dump(strlen($serialized));
