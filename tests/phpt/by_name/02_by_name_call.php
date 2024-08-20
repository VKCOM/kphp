@ok k2_skip
<?php

class HasSomeFields {
    /** @var B */
    public static $inst;

    /** @kphp-required */
    static public function getFieldInst(): B { return self::$inst; }
}

class EmtpyCtor {
    private int $idx;
    private static $count = 0;

    /** @var A */
    public static $inst;

    function __construct() { $this->idx = ++self::$count; }

    /** @kphp-required */
    static function sm(int $x = null, int $y = null) { echo "EmtpyCtor::sm($x,$y)\n"; }

    /** @kphp-required */
    static function getFieldInst(): A { return static::$inst; }

    static function createAInstance(int $value) { return new A($value); }
}

class A extends HasSomeFields {
    public int $value;

    function __construct(int $value) { $this->value = $value; }

    static function sm() { echo "A::sm()\n"; }

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
 * @return T::sm()
 */
function callSmNoArg($cn) {
    return $cn::sm();
}

/**
 * @kphp-generic T
 * @param class-string<T> $cn
 */
function callGetInstAndItsMethod($cn) {
    /** @var T::getFieldInst() */
    $inst = $cn::getFieldInst();
    echo "getFieldInst() of $cn returned ", get_class($inst), ": ", $inst->getInfoStr(), "\n";
}


callSmNoArg(EmtpyCtor::class);

callGetInstAndItsMethod(EmtpyCtor::class);
callGetInstAndItsMethod(HasSomeFields::class);

