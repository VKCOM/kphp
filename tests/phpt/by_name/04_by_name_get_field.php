@ok k2_skip
<?php

class HasSomeFields {
    public static $hello = 0;
    public static $f_arr = [0, 1];
    /** @var B */
    public static $inst;

    static public function testFieldArr() {
        testFieldArr(static::class);
    }
}

class EmtpyCtor {
    private int $idx;
    private static $count = 0;

    // same fields as HasSomeFields
    public static $hello = 1;
    public static $f_arr = ['1', '2'];
    /** @var A */
    public static $inst;

    function __construct() { $this->idx = ++self::$count; }

    function eMethod() { echo "EmtpyCtor {$this->idx}\n";}

    function getSelf(): self { return $this; }

    /** @kphp-required */
    static function sm(int $x = null, int $y = null) { echo "EmtpyCtor::sm($x,$y)\n"; }
}

class A extends HasSomeFields {
    public int $value;

    function __construct(int $value) { $this->value = $value; }

    function getInfoStr() { return "this is A value=$this->value"; }
}

class B {
    public A $a;
    private ?int $marker = null;

    function __construct(A $a, ?int $marker = null) { $this->a = $a; $this->marker = $marker; }

    function getInfoStr() { return "this is B marker={$this->marker}"; }
}


A::$inst = new B(new A(0), 111);
EmtpyCtor::$inst = new A(111);



/**
 * @kphp-generic T
 * @param class-string<T> $cn
 */
function printFieldHello($cn) {
    echo "hello of ", $cn, " = ", $cn::$hello, "\n";
}

/**
 * @kphp-generic T
 * @param class-string<T> $cn
 */
function incFieldHello($cn) {
    $add_5 = function(int &$int) { $int += 5; };
    $cn::$hello++;
    $cn::$hello += 5;
    $add_5($cn::$hello);
    ++$cn::$hello;
}

/**
 * @kphp-generic T
 * @param class-string<T> $klass
 */
function testFieldArr($klass) {
    $klass::$f_arr[1] += 2;
    $klass::$f_arr[] = [$klass::$f_arr[0]][0];
    echo 'f_arr of ', $klass, ': ', 'n=', count($klass::$f_arr), ', [1]=', $klass::$f_arr[1], "\n";
}

/**
 * @kphp-generic T
 * @param class-string<T> $klass
 */
function testFieldInst($klass) {
    /** @var T::inst */
    $inst = $klass::$inst;
    echo "inst of ", $klass, ": ", $inst->getInfoStr(), "\n";
}


printFieldHello(EmtpyCtor::class);
incFieldHello(EmtpyCtor::class);
printFieldHello(EmtpyCtor::class);

printFieldHello(A::class);
incFieldHello(A::class);
printFieldHello(A::class);
printFieldHello(HasSomeFields::class);

testFieldArr(EmtpyCtor::class);
HasSomeFields::testFieldArr();
A::testFieldArr();

testFieldInst(A::class);
testFieldInst(EmtpyCtor::class);

