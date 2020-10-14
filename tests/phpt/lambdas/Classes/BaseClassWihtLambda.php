<?php

namespace Classes;

class BaseClassWihtLambda {
    /**
     * @return int
     */
    public static function use_lambda() {
        $x = 10;
        $f = function() use ($x) {
            return $x;
        };

        return $f();
    }
}
