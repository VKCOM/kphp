@ok
<?php

abstract class Base {
    abstract public function foo();

    public function run_base() {
        var_dump("run_base");
    }

    public static function use_base(Base $b) {
        $b->foo();
    }
}

class Derived extends Base {
    public function foo() {
        var_dump("Derived");
        $this->run_base();
    }
}

class Derived2 extends Base {
    public function foo() {
        $this->run_base();
        var_dump("Derived2");
    }
}

Base::use_base(new Derived());
Base::use_base(new Derived2());

