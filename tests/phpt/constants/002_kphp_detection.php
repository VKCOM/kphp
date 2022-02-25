@ok
<?php

$expected = 'KPHP';
#ifndef KPHP
$expected = 'PHP';
#endif

if (defined('KPHP_COMPILER_VERSION')) {
  $actual = 'KPHP';
} else {
  $actual = 'PHP';
}

var_dump($actual === $expected);
