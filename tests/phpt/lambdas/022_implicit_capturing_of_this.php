@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function test_simple_capturing() {
    $a = new Classes\ImplicitCapturingThis(100);
    $a->run();
}

function test_capturing_in_second_level() {
    $a = new Classes\ImplicitCapturingThis(100);
    $a->capturing_in_lambda_inside_lambda();
}

function test_capturing_this_internal_functions() {
    $a = new Classes\ImplicitCapturingThis(2);
    $a->capturing_in_internal_functions();
}

function test_first_method_in_class() {
    $a = new Classes\ImplicitCapturingThis(2);
    $a->before_any_field();
}

function test_static_in_non_static_class() {
    Classes\ImplicitCapturingThis::static_inside_non_static_class();
}

function test_capturing_in_static_class() {
    Classes\StaticImplicitCapturingThis::run();
}

function test_capturing_in_static_class_internal_functions() {
    Classes\StaticImplicitCapturingThis::lambda_in_internal_functions();
}

test_simple_capturing();
test_capturing_in_second_level();
test_capturing_this_internal_functions();
test_first_method_in_class();
test_static_in_non_static_class();

test_capturing_in_static_class();
test_capturing_in_static_class_internal_functions();


class WithFCapturingThis {
    public $prop = 1;
    function f() {
        $f = function() { echo $this->prop; };
        $f();
    }
    function f2() {
        $f = fn() => print_r($this->prop);
        $f();
    }
    function f3() {
        $f = function() {
            $g = function() {
                $h = fn() => print_r($this->prop);
                $h();
            };
            $g();
        };
        $f();
        $f();
    }
}
(new WithFCapturingThis)->f();
(new WithFCapturingThis)->f2();
(new WithFCapturingThis)->f3();


