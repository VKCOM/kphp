@todo
/php type hint array mismatches with @param \?int\[\]/
<?php

/**
 * @param ?int[] $o
 */
function f(array $o) {
}

f();
