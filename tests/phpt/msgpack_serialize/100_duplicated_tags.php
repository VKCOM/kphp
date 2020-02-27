@kphp_should_fail
/kphp-serialized-field: 1 is already in use/
<?php

/** @kphp-serializable */
class A {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $x = 10;

    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $y = 10;
}

$a = new A();
