@ok
<?php

require_once "polyfills.php";

/** @kphp-inline */
function foo() {
    static $a = [];
    $a = [new Classes\A()];
    bar();
}

/** @kphp-inline */
function bar() {
    if (0) {
        foo();
    }
}

function run() {
    $a = [];
    bar();
}

run();
var_dump("OK");
