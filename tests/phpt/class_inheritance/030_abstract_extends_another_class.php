@ok
<?php

abstract class Base {
    abstract public function foo();
    /**
     * @kphp-infer
     * @param string $x
     */
    abstract public function bar($x);
}

class DerivedUnused extends Base {
    public function foo() {}
    /**
     * @kphp-infer
     * @param string $x
     */
    public function bar($x) {}
}

abstract class AbstractDerived extends Base {
    public function foo() {
        var_dump("AbstractDerived");
    }

    abstract public function abstract_derived_foo();
}

class Derived extends AbstractDerived {
    /**
     * @kphp-infer
     * @param string $x
     */
    public function bar($x) {
        var_dump("Derived".$x);
        $this->foo();
    }

    public function abstract_derived_foo() {
        var_dump("abstract_derived in derived");
    }
}

/**@var Base*/
$d = true ? new Derived() : new DerivedUnused();
$d->bar("asdf");
