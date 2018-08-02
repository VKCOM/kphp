<?php

namespace Classes;

class TestFieldClassParent {
    public static function getStaticClass() {
        return static::class;
    }

    public static function getSelfClass() {
        return self::class;
    }
}

