@kphp_should_fail k2_skip
/Called instance_serialize\(\) for class IntA, but it's not marked with @kphp-serializable/
<?php

require_once 'kphp_tester_include.php';

interface IntA {
}

/** @kphp-serializable */
class A implements IntA {
}

/**@var IntA*/
$a = new A();
instance_serialize($a);
