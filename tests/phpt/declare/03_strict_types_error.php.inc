<?php
/** @kphp-strict-types-enable */
declare(strict_types=1);

require_once 'kphp_tester_include.php';

var_dump(strcmp(54, 10));

function test_long2ip() {
  $i = 10;
  $s = '150';
  var_dump(long2ip(13)); // OK
  var_dump(long2ip($i)); // OK
  var_dump(long2ip((int)$s)); // OK

  $a = ['foo'];
  var_dump(long2ip($a)); // error
}

test_long2ip();
