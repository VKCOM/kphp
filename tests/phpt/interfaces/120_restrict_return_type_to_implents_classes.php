@kphp_should_fail
/Incorrect return type of function/
<?php

interface IA {
    /**
     * @return int
     */
    public function foo();
}

class A implements IA {
    public function foo() {
        return "asf";
    }
}

function foo(IA $ia) {
    return $ia->foo();
}

foo(new A());
