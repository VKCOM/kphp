<?php

namespace VK\Strings\Internals123;

class Append123 {
    static function concat($s1, $s2) {
        return $s1 . $s2;
    }

    static function notExportedF() {
        return self::concat("not", "exported");
    }
    static function notExportedF2() {
        return self::concat("not", "exported2");
    }
}
