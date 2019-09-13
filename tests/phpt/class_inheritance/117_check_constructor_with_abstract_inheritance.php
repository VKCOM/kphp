@kphp_should_fail
<?php

class AClass {
    public function __construct() {
        var_dump("AClass construct");
    }
}

abstract class Base extends AClass {
}

class Derived extends Base {

}

$d = new Derived();

