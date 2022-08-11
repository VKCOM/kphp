<?php

namespace VK\RPC\Utils123\Impl123;

class HasherImpl123 {
    static public function calcHash(string $s): int {
        return intval($s) ^ (2+4+8);
    }

    static public function calc2() {
        \VK\Strings\Internals123\Str123\StrSlice123::slice2('asdf');
    }
}
