@kphp_should_fail
/return string from A::foo/
/but it's declared as @return int/
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
