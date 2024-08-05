@ok k2_skip
<?php

class NoCtor {
    function nMethod() { echo "NoCtor\n";}
}

class EmtpyCtor {
    private int $idx;
    private static $count = 0;

    function __construct() { $this->idx = ++self::$count; }

    function eMethod() { echo "EmtpyCtor {$this->idx}\n";}

    function getSelf(): self { return $this; }

    /** @kphp-required */
    static function getFieldInst(): A { return static::$inst; }

    static function createAInstance(int $value) { return new A($value); }
}

class A {
    public int $value;

    function __construct(int $value) { $this->value = $value; }

    function aMethod() { echo "A value = {$this->value}\n"; }

    function getSelf(): self { return $this; }
}

class B {
    public A $a;
    private ?int $marker = null;

    function __construct(A $a, ?int $marker = null) { $this->a = $a; $this->marker = $marker; }

    function bMethod() { echo "B with a->value = {$this->a->value} marker = {$this->marker}\n"; }
}

class Joiner {
    function __construct(A $a, B $b) {
        echo "construct Joiner: a {$a->value}, b {$b->a->value}\n";
    }

    function jMethod() { }
}

class Str3 {
    public string $concat;
    function __construct(string $a, string $b = '', string $c = '') {
        $this->concat = "$a$b$c";
    }
    function printMe() {
        echo "Str3: $this->concat\n";
    }
}



/**
 * @kphp-generic T
 * @param class-string<T> $c
 * @return T
 */
function createTWithNoArg($c) {
    return new $c;
}

/**
 * @kphp-generic T
 * @param class-string<T> $c
 * @return T
 */
function createTWithInt($c) {
    return (new $c(1))->getSelf()->getSelf();
}

/**
 * @kphp-generic T, TArg
 * @param class-string<T> $c
 * @param TArg $arg
 * @return T
 */
function createTWithAnySingleArg($c, $arg) {
    return new $c($arg);
}

/**
 * @kphp-generic T, ...TArg
 * @param class-string<T> $c
 * @param TArg ...$args
 * @return T
 */
function createTWithAnyArgs($c, ...$args) {
    return new $c(...$args);
}

/**
 * @kphp-generic T, ...TArg
 * @param class-string<T> $c
 * @param TArg ...$strings
 */
function createPrependingStrAndPrint($c, ...$strings): void {
    $o = new $c(...['a', ...$strings]);
    $o->printMe();
}


createTWithNoArg(EmtpyCtor::class)->eMethod();
createTWithNoArg(EmtpyCtor::class)->eMethod();
createTWithNoArg(NoCtor::class)->nMethod();

$a = createTWithInt(A::class);
$a->aMethod();

createTWithAnySingleArg(A::class, 10)->aMethod();
createTWithAnySingleArg(B::class, new A(11))->bMethod();

createTWithAnyArgs(A::class, 20)->aMethod();
createTWithAnyArgs(B::class, new A(21))->bMethod();
createTWithAnyArgs(B::class, new A(22), -22)->bMethod();

createTWithAnyArgs(Joiner::class, new A(71), new B(new A(72)))->jMethod();
createTWithAnyArgs(Joiner::class, new A(81), new B(new A(82)))->jMethod();

createPrependingStrAndPrint(Str3::class);
createPrependingStrAndPrint(Str3::class, 'b');
createPrependingStrAndPrint(Str3::class, 'b', 'c');
