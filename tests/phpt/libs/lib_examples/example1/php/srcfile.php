<?php

echo "[example1] srcfile: global run\n";

/**
 * @kphp-lib-export
 * @param string $s
 * @return string
 */
function foo($s) {
    echo "[example1] srcfile: called foo('", $s, "')\n";
    return  "$s == $s";
}

/**
 * @kphp-lib-export
 * @return int
 */
function use_class_static_function() {
    echo "[example1] srcfile: called use_class_static_function()\n";
    echo "[example1] srcfile: call LibClasses\A::static_fun(): \n";
    echo LibClasses\A::static_fun(), "\n";
    return 0;
}
