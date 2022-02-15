@todo
/php type hint int mismatches with @param any/
<?php

/**
 * @param any $o
 */
function f(int $o) {
}

f();
