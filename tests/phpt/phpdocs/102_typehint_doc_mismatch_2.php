@todo
/php type hint bool mismatches with @param mixed/
<?php

/**
 * @param mixed $v
 */
function f(bool $v) {
}

f();
