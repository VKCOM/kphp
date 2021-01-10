@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/pass bool to argument \$b of f1/
/return bool from f2/
/pass bool\[\] to argument \$arr of f3/
/but it's declared as @param false\[\]/
<?php

/**
 * @param false $b
 */
function f1($b) {}

f1(false);
f1(true);

/**
 * @return false
 */
function f2(bool $b = true) { return $b; }

f2();

/** @param false[] $arr */
function f3(array $arr) {}

f3(['k' => false]);
f3(['l' => true]);
