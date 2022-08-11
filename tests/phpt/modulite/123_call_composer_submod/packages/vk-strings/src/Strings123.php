<?php

namespace VK\Strings;

class Strings123 {
    static public function concat($s1, $s2) {
        return Internals123\Append123::concat($s1, $s2);
    }
}
