@kphp_should_fail
/'case' expressions are available only in enum declaration/
<?php

class A {
    public function __construct() {
        echo "ctor";
    }
    case B;
}

new A();
