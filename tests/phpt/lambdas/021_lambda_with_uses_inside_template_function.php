@ok
<?php

require_once 'kphp_tester_include.php';

class A {
    function fa() { echo "fa\n"; }
}

class B {
    function fa() { echo "fb\n"; }
}

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

    $aih = new Classes\IntHolder(100);
    /** @var Classes\IntHolder */
    $aih_copy = $test_template_id_lambda($aih);
    var_dump($aih_copy->a);
}

$test_id_lambda = function ($argg): Classes\IntHolder {
    $f = function() use ($argg) {
        return $argg;
    };

    return $f();
};

test_lambda_with_uses_in_template_function();
test_lambda_with_uses_in_another_lambda_passed_as_arg($test_id_lambda);

/**
 * @kphp-generic T
 * @param T $a
 */
function withCapturingTpl($a) {
    $f = fn() => $a->fa();
    $f();
}
withCapturingTpl(new A);
withCapturingTpl(new B);

/**
 * @kphp-template $callArg
 */
function withCapturingPredicate(callable $predicate, $callArg) {
  $inner = function() use ($predicate, $callArg) {
    $predicate($callArg);
  };
  $inner();
}
withCapturingPredicate(function(A $a) { $a->fa(); }, new A);
withCapturingPredicate(function(B $b) { echo get_class($b), "\n"; }, new B);
withCapturingPredicate(function($v) { var_dump($v); }, 100);


class Pr_A {
    function getArg2() { return 10; }
    function getSummand() { return 10; }
}
class Pr_B {
    function getArg2() { return 100; }
    function getSummand() { return 100; }
}
class Pr_Demo {
    public int $multiplier = 7;

    /** @kphp-template $obj */
    function tplE($obj, int $arg) {
        $x = function($arg) use($obj) {
            return function($arg2) use($arg, $obj) {
                echo ($arg + $arg2) * $this->multiplier + $obj->getSummand(), "\n";
            };
        };
        $x($arg)($obj->getArg2());
    }
    /** @kphp-template $obj */
    function tplE2($obj, int $arg) {
        $x = fn($arg) => fn($arg2) => print_r(($arg + $arg2) * $this->multiplier + $obj->getSummand() . "\n");
        $x($arg)($obj->getArg2());
    }
}

(new Pr_Demo)->tplE(new Pr_A, 1);
(new Pr_Demo)->tplE(new Pr_B, 1);
(new Pr_Demo)->tplE2(new Pr_B, 1);


// 
// test lambda in global scope
//
$ih = new Classes\IntHolder(10);
$ih_copy = $test_id_lambda($ih);
var_dump($ih_copy->a);

$aih = new Classes\IntHolder(100);
$aih_copy = $test_id_lambda($aih);
var_dump($aih_copy->a);
