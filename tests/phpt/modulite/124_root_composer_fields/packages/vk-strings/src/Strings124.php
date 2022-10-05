<?php

namespace VK\Strings;

class Strings124 {
    static public int $count = 0;

    static public function concat($s1, $s2) {
        Impl124\Hidden124::$arr[] = 1;
        return Impl124\Append124::appendStr($s1, $s2);
    }
}
