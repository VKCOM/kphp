<?php

namespace Classes\Inheritance;

class SimpleBase {
    public static function test1() {
        echo "test1 Base\n";
    }

    public static function test2() {
        SimpleChild::test5();
    }

    static public function test3() {
        echo "test3 Base\n";
    }

    static public function test4() {
        SimpleChild::test7();
    }
}
