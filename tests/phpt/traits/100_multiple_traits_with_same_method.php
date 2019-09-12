@kphp_should_fail
<?php

trait Tr1 {
    public function foo() {}
}

trait Tr2 {
    public function foo() {}
}

class A {
    use Tr1;
    use Tr2;
}

$a = new A();
