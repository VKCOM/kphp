@kphp_should_fail
/Class Tr/
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

