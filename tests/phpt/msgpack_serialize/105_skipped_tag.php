@kphp_should_fail
/php-serialized-field is required for field: x/
<?php

require_once 'polyfills.php';

/** 
 * @kphp-serializable
 **/
class A {
    /**
     * @var int
     */
    public $x = 10;

    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $y = 10;
}

$a = new A();
instance_serialize($a);
