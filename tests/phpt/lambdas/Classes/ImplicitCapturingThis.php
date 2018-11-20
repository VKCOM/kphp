<?php

namespace Classes;

class ImplicitCapturingThis
{
    var $a;

    public function __construct($a = 0) {
        $this->a = $a;
    }

    public function run() {
        $f = function ($x) {
            return $x + $this->a;
        };

        var_dump($f(100) + 10);
    }

    public function capturing_in_lambda_inside_lambda() {
        $f = function () {
            $f2 = function () {
                return $this->a;
            };

            return $f2();
        };

        var_dump($f());
    }
}
