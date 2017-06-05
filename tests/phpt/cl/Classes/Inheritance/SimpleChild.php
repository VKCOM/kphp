<?php

namespace Classes\Inheritance;

class SimpleChild extends SimpleBase {
    public static function test3() {
        echo "Child\n";
    }

    public static function test4() {
        SimpleBase::test1();
    }
}
