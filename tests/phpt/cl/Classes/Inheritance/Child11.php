<?php

namespace Classes\Inheritance;

class Child11 extends Child1 {
    const C1 = 'Child11Const1';
    const C2 = 'Child11Const2';

    private static $f2 = 'Field2';
    static private $f4 = 'Field4';

    static private function test1() {

    }

    protected static function test2() {

    }

    private static function test3() {

    }

    static protected function test4() {

    }

    public static function const_test1() {
        echo self::C1."\n";
        echo static::C1."\n";
        echo parent::C1."\n";
    }

    static public function const_test2() {
        echo Child11::C1."\n";
        echo Child1::C1."\n";
        echo Base::C1."\n";
        echo "\n";
        echo self::C1."\n";
        echo static::C1."\n";
        echo parent::C1."\n";
        parent::const_test2();
    }

    static public function const_test3() {
        echo self::C2."\n";
        echo static::C2."\n";
        echo parent::C2."\n";
    }

    public static function const_test4() {
        echo Child11::C2."\n";
        echo Child1::C2."\n";
        echo Base::C2."\n";
        echo "\n";
        echo self::C2."\n";
        echo static::C2."\n";
        echo parent::C2."\n";
        parent::const_test4();
    }
}
