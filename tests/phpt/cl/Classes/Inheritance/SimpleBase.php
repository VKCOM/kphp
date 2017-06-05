<?php

namespace Classes\Inheritance;

class SimpleBase {
    public static function test1() {
        echo "Base\n";
    }

    public static function test2() {
        SimpleChild::test3();
    }
}
