@kphp_should_fail
/Modification of const variable: matches/
<?php

// PHP supports this special behavior to avoid excessive warnings
// when variables are modified by reference (from docs):
//
//   $fn = fn($str) => preg_match($regex, $str, $matches) && ($matches[1] % 7 == 0)
//
//   Here $matches is populated by preg_match() and needn't exist prior to the call.
//   We would not want to generate a spurious undefined variable notice in this case.
//
// TODO: make it work? Do we need it?
// An example below can be re-written with a normal lambda.

$regex = '/a./';
$fn = fn($str) => preg_match($regex, $str, $matches) && ($matches[0] === "a1");
var_dump($fn('a1a2'));
var_dump($fn('b1b2'));

// $matches is not defined in the global scope
