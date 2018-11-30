@ok
<?php

require_once("Classes/autoload.php");

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
