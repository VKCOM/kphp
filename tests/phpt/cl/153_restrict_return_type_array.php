@ok
<?php

interface C {
    public function foo();
}

class A implements C {
    public function foo() {
        var_dump("A");
    }
}

class B implements C {
    public function foo() {
        var_dump("B");
    }
}

/**
 * @return C[]
 */
function foo() {
    $a = [new A(), new A()];
    return $a;
}

/**
 * @return A[]
 */
function get_empty() {
    return [];
}

$x = foo();
$x[] = new B();
foo()[0]->foo();
foo()[1]->foo();

$empty_array = get_empty();
$empty_array[] = new A();

