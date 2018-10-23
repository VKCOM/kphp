<?php

namespace Classes\Inheritance;

class SimpleChild extends SimpleBase {
    public static function test5() {
        echo "test5 Child\n";
    }

    public static function test6() {
        SimpleBase::test1();
    }

    static public function test7() {
        echo "test7 Child\n";
    }

    static public function test8() {
        SimpleBase::test3();
    }
}
