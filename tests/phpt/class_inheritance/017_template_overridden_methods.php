@ok
<?php

class TempA { 
    /**
     * @param string $s
     */
    public function run($s) { var_dump("TempA::$s"); }
}

class TempB {
    /**
     * @param string $s
     */
    public function run($s) { var_dump("TempB::$s"); }
}

class B {
    public function foo(callable $f) {
        $f(20);
    }

    /**
     * @kphp-template T $x
     */
    public function template_bar($x) {
        $x->run("B");
    } 
}

class D extends B {
    public function foo(callable $f) {
        $f(123);
    }

    /**
     * @kphp-template T $x
     */
    public function template_bar($x) {
        $x->run("D");
    } 
}

$call_foo = function (B $b) {
    $b->foo(
        function($x) use ($b) { 
            var_dump(get_class($b).":: lambda: $x"); 
        }
    );

    $b->foo(
        function($x) use ($b) { 
            var_dump(get_class($b).":: lambda2: $x"); 
        }
    );

    $b->template_bar(new TempA());
    $b->template_bar(new TempB());
};

$call_foo(new B());
$call_foo(new D());
