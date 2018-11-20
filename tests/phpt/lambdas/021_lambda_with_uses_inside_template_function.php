@todo
<?php

require_once("Classes/autoload.php");

/**
 * @kphp-template T $arg
 * @kphp-return T
 */
function test_template_id($arg) {
    $f = function($x) use ($arg) {
        return $arg;
    };
    $f(10);

    return $f(10);
}

function test_lambda_with_uses_in_template_function() {
    $ih = new Classes\IntHolder(10);
    /** @var Classes\IntHolder */
    $ih_copy = test_template_id($ih);
    var_dump($ih_copy->a);

    $aih = new Classes\AnotherIntHolder(100);
    /** @var Classes\AnotherIntHolder */
    $aih_copy = test_template_id($aih);
    var_dump($aih_copy->a);

    var_dump(test_template_id(900));
}

function test_lambda_with_uses_in_another_lambda_passed_as_arg(callable $test_template_id_lambda) {
    $ih = new Classes\IntHolder(10);
    /** @var Classes\IntHolder */
    $ih_copy = $test_template_id_lambda($ih);
    var_dump($ih_copy->a);

    $aih = new Classes\AnotherIntHolder(100);
    /** @var Classes\AnotherIntHolder */
    $aih_copy = $test_template_id_lambda($aih);
    var_dump($aih_copy->a);
}

$test_template_id_lambda = function ($arg) {
    $f = function() use ($arg) {
        return $arg;
    };

    return $f();
};

test_lambda_with_uses_in_template_function();
test_lambda_with_uses_in_another_lambda_passed_as_arg($test_template_id_lambda);

// 
// test lambda in global scope
//
$ih = new Classes\IntHolder(10);
/** @var Classes\IntHolder */
$ih_copy = $test_template_id_lambda($ih);
var_dump($ih_copy->a);

$aih = new Classes\AnotherIntHolder(100);
/** @var Classes\AnotherIntHolder */
$aih_copy = $test_template_id_lambda($aih);
var_dump($aih_copy->a);
