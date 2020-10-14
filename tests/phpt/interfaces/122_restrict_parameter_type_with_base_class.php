@kphp_should_fail
<?php

class Base { public function base_foo() {} }
class Derived extends Base {}

interface IA {
    /**
     * @param Base $x
     * @return int
     */
    public function foo($x);
}

class A implements IA {
    /**
     * @param Derived $x
     * @return int
     */
    public function foo($x) {
        return 10;
    }
}

$a = function (IA $ia) { return $ia->foo(new Derived()); };
$a(new A());

