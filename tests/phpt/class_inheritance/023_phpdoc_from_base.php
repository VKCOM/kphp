@ok
<?php

class A {
    public function foo() { var_dump("A"); }
}

class Base {
    /**
     * @var A
     */
    var $errors;

    /** @return A */
    public function get_a() {
        return new A();
    }
}

class Derived extends Base {
    public function bar() {
        $this->errors->foo();
        $this->get_a()->foo();
    }
}

$b = new Derived;
$b->errors = new A();
$b->bar();

