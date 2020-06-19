@ok
<?php

class GrandPa {
    /** @var int */
    public $grand_pa;

    public function __construct($x) {
        $this->grand_pa = $x + 2;
        var_dump("GrandPa", $this->grand_pa);
    }

    public function grand_foo() {
        var_dump("grand_foo");
    }
}

class Parent_ extends GrandPa {
    /** @var int */
    public $parent;

    public function __construct($x) {
        $this->parent = $x + 1;
        var_dump("Parent", $this->parent);
    }

    public function parent_foo() {
        var_dump("parent_foo");

        parent::grand_foo();
        $this->grand_foo();
        self::grand_foo();
    }
}

class Derived extends Parent_ {
    /** @var int */
    public $derived;

    public function __construct($x) {
        GrandPa::__construct($x);
        parent::__construct($x);
        $this->derived = $x;

        var_dump("Derived", $this->derived);
    }

    public function foo() {
        var_dump("foo");

        $this->parent_foo();

        $this->grand_foo();
        GrandPa::grand_foo();
        parent::grand_foo();
        self::grand_foo();
    }
}

$d = new Derived(100);
$d->foo();
