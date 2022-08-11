<?php

namespace Feed113;

class Post113 {
    static $ONE = 1;
    static $TWO = 2;
    static $THREE = 3;

    static function demo() {
        echo self::$ONE, self::$TWO, "\n";
        echo Infra113\Strings113::$str_name, "\n";
        echo Infra113\Strings113::$str_hidden, "\n";
        Infra113\Hidden113::$HIDDEN_1++;
    }
}
