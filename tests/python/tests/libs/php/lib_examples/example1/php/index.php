<?php

#ifndef KPHP
require_once 'lib_autoload.php';
#endif

/**
 * @kphp-lib-export
 */
function example1_run() {
echo "[example1] index: global run\n";

echo "[example1] index: require_once 'srcfile.php'\n";
require_once 'srcfile.php';
require_once 'srcfile2.php';
}

/**
 * @kphp-lib-export
 * @return string
 */
function get_str() {
    static $x = "hello world";
    return $x;
}

/**
 * @kphp-lib-export
 * @param int $x
 * @param int $y
 * @return int
 */
function minus1($x, $y) {
    echo "[example1] index: called minus1(", $x, ", ", $y, ")\n";
    return $x - $y;
}

/**
 * @kphp-lib-export
 * @param float $x
 * @return float
 */
function pow2($x = 1.23) {
    echo "[example1] index: called pow2(", $x, ")\n";
    return $x ** 2;
}

/**
 * @kphp-lib-export
 * @return int
 */
function use_class_static_field() {
    echo "[example1] index: called use_class_static_field()\n";

    echo "[example1] index: LibClasses\A::field \n";
    echo LibClasses\A::$field, "\n";
    return 0;
}

/**
 * @kphp-lib-export
 * @return int
 */
function use_class_new() {
    echo "[example1] index: called use_class_new()\n";

    echo "[example1] index: a = new LibClasses\A\n";
    $a = new LibClasses\A;

    echo "[example1] index: a->value\n";
    echo $a->value, "\n";

    echo "[example1] index: a->set_value('hello world!')\n";
    $a->set_value("hello world!");

    echo "[example1] index: a->get_value()\n";
    echo $a->get_value(), "\n";

    return 0;
}
