<?php

namespace Feed113\Infra113;

class Strings113 {
    static string $str_name = 'name';
    static $str_hidden = 'hidden';

    static function demo() {
        echo self::$str_name, ' ', self::$str_hidden, ' ', Hidden113::$HIDDEN_1, Hidden113::$HIDDEN_2;
        Hidden113::demo();
    }
}
