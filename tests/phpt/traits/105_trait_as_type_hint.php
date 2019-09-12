@kphp_should_fail
/You may not use trait/
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

