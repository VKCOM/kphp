@kphp_should_fail
<?php

interface C {
}

class A implements C {
    public function foo() {}
}

class B implements C {
    public function foo() {}
}

/**
 * @return A
 */
function new_b() {
    return new B();
}

function test() {
    new_b()->foo();;
}

test();

