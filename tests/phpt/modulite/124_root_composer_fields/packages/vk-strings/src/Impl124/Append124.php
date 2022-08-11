<?php

namespace VK\Strings\Impl124;

class Append124 {
    public static bool $inited = false;
    public static int $more = 0;

    static public function appendStr(string $s1, string $s2) {
        return $s1 . $s2;
    }
}
