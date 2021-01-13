@kphp_should_fail
/Do not use \|bool in phpdoc, use \|false instead/
<?php

/** @param int|bool $a */
function f($a) {}

f(3);
