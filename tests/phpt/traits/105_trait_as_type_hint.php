@kphp_should_fail
/Using trait Tr as a type is invalid/
<?php

trait Tr {
    public function foo() {}
}

class A {
    use Tr;
}

function run(Tr $tr) {
    $tr->foo();
}

run(new A());

