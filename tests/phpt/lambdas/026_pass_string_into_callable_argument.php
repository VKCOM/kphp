@ok
<?php

require_once("Classes/autoload.php");

/**
 * @kphp-required
 */
function my_callback($x) { 
    var_dump($x);
}

function use_callback(callable $callback, $x) {
    $callback($x);
}

function test_global_callbacks() {
    $php_ver = 7;
#ifndef KittenPHP
    $php_ver = (int)phpversion();
#endif

    // $a = new Classes\IntHolder(10);
    // unsupported now
    // use_callback([$a, "f"], 1);

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
 
function test_callbacks_in_classes() {
    Classes\UseCallbacksAsString::run_static();
    Classes\UseCallbacksAsString::run_me_static();
}

Classes\IntHolder::f_static(10);
test_global_callbacks();

test_callbacks_in_classes();

Classes\NewClasses\A::f_print_static(10);
Classes\UseCallbacksAsString::use_callback_static(['Classes\NewClasses\A', 'f_print_static']);

