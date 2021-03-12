<?php
/** @kphp-strict-types-enable */
declare(strict_types=1);

// If we remove strict_types=1, this could would work

/**
 * @kphp-infer cast
 * @param string $s
 */
function f($s) {}

f(24);
