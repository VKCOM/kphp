@kphp_should_fail
/Unknown function \\VK\\funcname/
/Unknown function \\VK\\Utils\\funcname/
<?php

use function VK\funcname as funcname2;
use function VK\Utils\funcname as funcname3;

function funcname() { return __FUNCTION__; }

function test() {
  var_dump(funcname());
  var_dump(funcname2());
  var_dump(funcname3());
}

test();
