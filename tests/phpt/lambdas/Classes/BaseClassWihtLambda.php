<?php

namespace Classes;

class BaseClassWihtLambda {
    public static function use_lambda() {
        $x = 10;
        $f = function() use ($x) {
            return $x;
        };

        return $f();
    }
}
