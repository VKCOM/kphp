<?php

namespace Utils111;

class Hidden111 {
    static function demo() {
        self::demo2();
    }

    static function demo2() {
        if(0) self::demo();
        if(0) Strings111::normal();
    }
}
