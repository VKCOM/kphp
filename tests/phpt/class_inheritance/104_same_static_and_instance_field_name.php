@kphp_should_fail
<?php

class B {
    var $x = 20;
}

class D extends B {
    public static $x = 123;
}

$d = new D();

