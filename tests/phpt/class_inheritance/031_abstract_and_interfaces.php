@ok
<?php

interface I {
    public function i_foo();
}

abstract class Base implements I {
    public $x = 10;
    public function base_foo() {}
}

class Derived extends Base {
    public function i_foo() {
        var_dump("Derived_i_foo".$this->x);
    }

    public function base_foo() {
        var_dump("Derived_base_foo");
    }
}

class IClass implements I {
    public function i_foo() {
        var_dump("IClass_i_foo");
    }
}

function run_i_foo(I $impl) {
    $impl->i_foo();
}

function run_base_foo(Base $impl) {
    $impl->i_foo();
    $impl->base_foo();
}

run_i_foo(new IClass());
run_i_foo(new Derived());

run_base_foo(new Derived());
