@ok
<?php

class B {
    const NUM = 1;

    public function foo(callable $f) {
        $f(20);
    }

    static function tplStaticNooverride(callable $f) {
        $f(static::NUM);
    }

    static function tplStaticOverridden(callable $f) {
        $f(static::NUM);
    }

    static function wrapperAbove(callable $f) {
        static::tplStaticOverridden($f);
    }
}

class D extends B {
    const NUM = 2;

    // a template method foo() is not overridden anywhere in children, it's ok
    // (but overriding instance template methods is unsupported)

    // but overriding static template methods is'ok
    static function tplStaticOverridden(callable $f) {
        $f(-100);
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

};

$call_foo(new B());
$call_foo(new D());

B::tplStaticNooverride(fn($x) => print_r($x . "\n"));
D::tplStaticNooverride(fn($x) => print_r($x . "\n"));
B::tplStaticOverridden(fn($x) => print_r($x . "\n"));
D::tplStaticOverridden(fn($x) => print_r($x . "\n"));
B::wrapperAbove(fn($x) => print_r($x . "\n"));
D::wrapperAbove(fn($x) => print_r($x . "\n"));
