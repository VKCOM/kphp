@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/pass boolean to argument \$b of f1/
/return boolean from f2/
/pass array< boolean > to argument \$arr of f3/
/but it's declared as @param array< false >/
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
