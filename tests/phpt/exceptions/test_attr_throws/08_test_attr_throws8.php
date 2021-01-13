@ok
<?php

/**
 * @kphp-throws RuntimeException
 */
function f() {
    try {
        g(false);
    } catch (RuntimeException $e) {
    }
    g(false);
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
