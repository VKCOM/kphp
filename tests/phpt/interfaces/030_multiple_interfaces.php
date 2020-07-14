@ok
<?php
interface IFoo { public function foo(int $x) : void; }

interface IBar { public function bar() : int; }

class FooBar implements IFoo, IBar {
    public function foo(int $x) : void {
        var_dump("FooBar::foo({$x})");
    }

    /**
     * @kphp-infer
     * @return int
     */
    public function bar() : int {
        var_dump("FooBar::bar()");
        return 536;
    }
}

class Barr implements IBar {
    /**
     * @kphp-infer
     * @return int
     */
    public function bar() : int {
        var_dump("Bar::bar()");
        return 635;
    }
}

function run_bar(IBar $ibar) { $ibar->bar(); }

function run_foo(IFoo $ifoo) { $ifoo->foo(536); }

$foo_bar = new FooBar();
$bar = new Barr();

run_foo($foo_bar);
run_bar($foo_bar);
run_bar($bar);
