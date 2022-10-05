<?php

namespace VK\Strings;

class StringErrBase007 {
    const PREFIX = 'base prefix';

    static function getCurDesc() {
        return 'base 007';
    }

    static function printCurDesc() {
        $desc = static::getCurDesc();
        echo $desc, ' (', static::PREFIX, ')', "\n";
    }
}
