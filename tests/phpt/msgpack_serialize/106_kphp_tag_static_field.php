@kphp_should_fail k2_skip
/kphp-serialized-field is allowed only for instance fields: y/
<?php

require_once 'kphp_tester_include.php';

/** 
 * @kphp-serializable
 **/
class A {
    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public static $y = 10;

    public function __construct() {}
}

$a = new A();
instance_serialize($a);
