@kphp_should_fail k2_skip
/bad kphp-serialized-field: ''/
<?php

require_once 'kphp_tester_include.php';

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
