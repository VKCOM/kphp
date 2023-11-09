<?php

namespace VK\Strings;

class StringErrBase014 {
    const PREFIX = 'base prefix';

    static function getCurDesc() {
        return 'base 014';
    }

    static function printCurDesc() {
        $desc = static::getCurDesc();
        echo $desc, ' (', static::PREFIX, ')', "\n";
    }
}
