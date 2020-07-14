<?php

namespace Classes;

class TestFieldClassParent {
    /**
     * @kphp-infer
     * @return string
     */
    public static function getStaticClass() {
        return static::class;
    }

    /**
     * @kphp-infer
     * @return string
     */
    public static function getSelfClass() {
        return self::class;
    }
}

