@kphp_should_fail
/kphp-serialized-field is allowed only for instance fields: y/
<?php

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
