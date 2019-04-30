@kphp_should_fail
/Modification of const/
<?php

class A {
    public $x = 20;
    public function foo() {
        $this = new A();
    }
}

$a = new A();
$a->foo();


