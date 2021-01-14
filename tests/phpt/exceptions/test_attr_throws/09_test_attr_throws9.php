@ok
<?php

/**
 * @kphp-throws RuntimeException
 */
function f() {
    g(false);
    try {
        g(false);
    } catch (RuntimeException $e) {
    }
}

/**
 * @kphp-throws RuntimeException
 */
function g($cond) {
    if ($cond) {
        throw new RuntimeException();
    }
}

f();
