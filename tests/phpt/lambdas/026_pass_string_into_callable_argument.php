@ok
<?php

require_once 'kphp_tester_include.php';

/**
 * @kphp-required
 */
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
#ifndef KPHP
    $php_ver = (int)phpversion();
#endif

    use_callback('my_callback', -1);

    use_callback(['\Classes\IntHolder', "f_static"], 0);
    use_callback(['Classes\IntHolder', 'f_static'], 2);

    if ($php_ver >= 7) {
        use_callback('\Classes\IntHolder::f_static', 1);
    } else {
        use_callback(['\Classes\IntHolder', 'f_static'], 1);
    }

    array_map('\Classes\IntHolder::f_static', [3, 4, 5]);

    use_callback(function($x) { var_dump(1000 + $x); }, 1);
}
 
function test_callbacks_in_static_classes() {
    Classes\UseCallbacksAsString::run_static();
    Classes\UseCallbacksAsString::run_me_static();
}

/**
 * @kphp-required
 */
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
}

Classes\IntHolder::f_static(10);
test_global_callbacks();

test_callbacks_in_static_classes();

Classes\NewClasses\A::f_print_static(10);
Classes\UseCallbacksAsString::use_callback_static(['Classes\NewClasses\A', 'f_print_static']);

test_callbacks_in_classes();
