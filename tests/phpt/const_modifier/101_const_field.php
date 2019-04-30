@kphp_should_fail
/Modification of const/
<?php

class A {
    /**  @kphp-const */
    public $x = 10;
}

$a = new A();
$a->x = 20;


