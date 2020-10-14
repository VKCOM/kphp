<?php

namespace Classes;

class TestFieldClassParent {
    /**
     * @return string
     */
    public static function getStaticClass() {
        return static::class;
    }

    /**
     * @return string
     */
    public static function getSelfClass() {
        return self::class;
    }
}

