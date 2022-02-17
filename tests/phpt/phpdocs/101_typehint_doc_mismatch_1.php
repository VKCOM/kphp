@todo
/php type hint array mismatches with @return tuple\(int\)/
<?php

/** @return tuple(int) */
function f(): array {
}

f();
