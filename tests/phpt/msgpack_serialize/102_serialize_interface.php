@kphp_should_fail
<?php

/** @kphp-serializable */
interface IntA {
}

class A implements IntA {
}

$a = new A();
