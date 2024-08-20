@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

/** @kphp-serializable **/
class A {
    /**
     * @kphp-serialized-field 1
     * @kphp-serialized-float32
     * @var float
     */
    public $x = 20.123;
}

/** @kphp-serializable **/
class B {
    /**
     * @kphp-serialized-field 1
     * @var float
     */
    public $x = 20.123;
}

$byte_for_cnt_elements_in_array = 1;
$byte_for_field_tag = 1;

$bytes_for_float = 1 + 8;
$bytes_for_float32 = 1 + 4;

var_dump(strlen(instance_serialize(new B)) == $byte_for_cnt_elements_in_array + $byte_for_field_tag + $bytes_for_float);
var_dump(strlen(instance_serialize(new A)) == $byte_for_cnt_elements_in_array + $byte_for_field_tag + $bytes_for_float32);
