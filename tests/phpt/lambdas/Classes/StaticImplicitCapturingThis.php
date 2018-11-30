<?php

namespace Classes;

class StaticImplicitCapturingThis
{
    public static function run() {
        $f = function ($x) {
            return $x;
        };

        var_dump($f(100) + 10);
    }

    public static function lambda_in_internal_functions() {
        $a = [1, 2, 3];
        $a = array_filter($a, function($x) { return $x > 2; });

        var_dump($a);
    }
}
