@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

class A {
    function fa($x) { return $x + 1; }
    function __toString(): string { return "A"; }
}

function getA() { return new A; }

function myArrayMap(callable $cb, array $arr) {
    $result = [];
    foreach ($arr as $k => $v) {
        $result[$k] = $cb($v);
    }
    return $result;
}

/**
 * @kphp-required
 * @param int $x
 */
function my_callback($x) { 
    var_dump($x);
}

function use_callback(callable $callback, $x) {
    $callback($x);
}

function test_global_callbacks() {
    $php_ver = 7;

    use_callback('my_callback', -1);

    use_callback(['\\Classes\\IntHolder', "f_static"], 0);
    use_callback(['Classes\\IntHolder', 'f_static'], 2);

    if ($php_ver >= 7) {
        use_callback('\\Classes\\IntHolder::f_static', 1);
    } else {
        use_callback(['\\Classes\\IntHolder', 'f_static'], 1);
    }

    array_map('\\Classes\\IntHolder::f_static', [3, 4, 5]);

    use_callback(function($x) { var_dump(1000 + $x); }, 1);
}
 
function test_callbacks_in_static_classes() {
    Classes\UseCallbacksAsString::run_static();
    Classes\UseCallbacksAsString::run_me_static();
}

/**
 * @kphp-required
 * @return int
 */
function my_callback2() { 
    return 100;
}

function test_callbacks_in_classes() {
    $use_it = new Classes\UseCallbacksAsString();
    $res = $use_it->use_callback(9000, [$use_it, 'get_aaa']);
    var_dump($res);

    $res = $use_it->use_callback(9000, [$use_it, 'get_bbb']);
    var_dump($res);

    $res = $use_it->use_callback(9000, 'my_callback2');
    var_dump($res);

    $r1 = array_map([getA(), 'fa'], [1,2,3]);
    var_dump($r1);
    $r2 = myArrayMap([getA(), 'fa'], [1,2,3]);
    var_dump($r2);
    $r3 = array_map([new A, 'fa'], [1,2,3]);
    var_dump($r3);
    $r4 = myArrayMap([new A, 'fa'], [1,2,3]);
    var_dump($r4);
}

Classes\IntHolder::f_static(10);
test_global_callbacks();

test_callbacks_in_static_classes();

Classes\NewClasses\A::f_print_static(10);
Classes\UseCallbacksAsString::use_callback_static(['Classes\NewClasses\A', 'f_print_static']);

test_callbacks_in_classes();



class Caller1 {
    public $state = 9;
    static function callAndEcho(callable $arg) {
        echo "result ", $arg(), "\n";
    }
    /** @kphp-required */
    static function testCb() { return "Caller1 testCb"; }
    function instCb() { return $this->state; }
}
Caller1::callAndEcho(fn() => 1);
Caller1::callAndEcho(fn() => new A);

Caller1::callAndEcho('php_sapi_name');
Caller1::callAndEcho('Caller1::testCb');
Caller1::callAndEcho([Caller1::class, 'testCb']);
$caller1_inst = new Caller1;
Caller1::callAndEcho([$caller1_inst, 'instCb']);

/** @var callable */
$f_php_sapi_name = 'php_sapi_name';
Caller1::callAndEcho($f_php_sapi_name);
/** @var callable */
$f_static_testCb_1 = 'Caller1::testCb';
Caller1::callAndEcho($f_static_testCb_1);
/** @var callable */
$f_static_testCb_2 = [Caller1::class, 'testCb'];
Caller1::callAndEcho($f_static_testCb_2);
$caller1_inst = new Caller1;
/** @var callable */
$f_instCb = [$caller1_inst, 'instCb'];
Caller1::callAndEcho($f_instCb);
$caller1_inst->state = 10;
Caller1::callAndEcho($f_instCb);
