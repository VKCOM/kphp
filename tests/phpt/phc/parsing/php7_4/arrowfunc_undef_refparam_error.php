@kphp_should_fail k2_skip
/\$matches is declared but never written; please, provide a default value/
<?php

// PHP supports this special behavior to avoid excessive warnings
// when variables are modified by reference (from docs):
//
//   $fn = fn($str) => preg_match($regex, $str, $matches) && ($matches[1] % 7 == 0)
//
//   Here $matches is populated by preg_match() and needn't exist prior to the call.
//   We would not want to generate a spurious undefined variable notice in this case.
//
// Here in KPHP, $matches is auto-captured and tries to be provided from an outer scope: L$xxx$$__construct($matches).
// Cause $matches doesn't exist, it's inferred as Unknown, leading to an error.
// We aren't going to fix this fabricated special case, as it could be rewritten with a normal lambda.

$regex = '/a./';
$fn = fn($str) => preg_match($regex, $str, $matches) && ($matches[0] === "a1");
var_dump($fn('a1a2'));
var_dump($fn('b1b2'));

