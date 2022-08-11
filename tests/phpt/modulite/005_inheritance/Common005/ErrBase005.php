<?php

namespace Common005;

class ErrBase005 {
    const PREFIX = 'base prefix';

    static function getCurDesc() {
        return 'base 005';
    }

    static function printCurDesc() {
        $desc = static::getCurDesc();
        echo $desc, ' (', static::PREFIX, ')', "\n";
    }
}
