@kphp_should_fail
/You may not serialize class without @kphp-serializable tag IntA/
<?php

require_once 'polyfills.php';

interface IntA {
}

/** @kphp-serializable */
class A implements IntA {
}

/**@var IntA*/
$a = new A();
instance_serialize($a);
