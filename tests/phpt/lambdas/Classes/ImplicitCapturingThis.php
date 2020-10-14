<?php

namespace Classes;

class ImplicitCapturingThis
{
    public function before_any_field() {
        $f = function() {
            var_dump($this->a);
        };

        $f();
    }

    /** @var int */
    var $a;

    /**
     * @param int $a
     */
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
            // $f2 = function () {
                return $this->a;
            // };

            // return $f2();
        };

        var_dump($f());
    }

    public function capturing_in_internal_functions() {
        $a = [1, 2, 3, 4];
        $a = array_filter($a, function($x) { return $x > $this->a; });
        var_dump($a);
    }

    public static function static_inside_non_static_class() {
        $f = function() {
            return 10;
        };

        var_dump($f());
    }

    public function pass_field_as_argument_of_lambda(callable $callback) {
        $callback($this->a);
    }

    /**
     * @kphp-template $field
     */
    public function pass_template_fields(callable $callback, $field) {
       $res = $callback();
       $a = $field->a;
       $b = $res->a;
       var_dump($a + $b);
    }
}
