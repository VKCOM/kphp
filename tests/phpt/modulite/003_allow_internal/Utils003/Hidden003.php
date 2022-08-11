<?php

namespace Utils003;

class Hidden003 {
    static function demo() {
        self::demo2();
        Impl003\Hasher003::calc2();
    }

    static function demo2() {
        if(0) self::demo();
        if(0) Strings003::normal();
    }
}
