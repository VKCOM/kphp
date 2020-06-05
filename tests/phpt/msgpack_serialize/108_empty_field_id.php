@kphp_should_fail
/bad kphp-serialized-field: ''/
<?php

require_once 'polyfills.php';

/** 
 * @kphp-serializable
 **/
class A {
    /**
     * @kphp-serialized-field 
     * @var int
     */
    public $y = 10;
}

$a = new A();
$serialized = instance_serialize($a);
