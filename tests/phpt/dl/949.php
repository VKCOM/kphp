@ok benchmark k2_skip
<?php

function my_throw ($msg, $code) {
  throw new Exception ($msg, $code);
}

function report_exception (Exception $e, int $line) {
  echo "Got exception [". $e->getMessage() . "] code [" . $e->getCode() . "] at " . $line . "\n";
}
function smth() {
  my_throw ("D", 4);
  var_dump ("OPPA!");
}

function test() {
  try {
    throw new Exception ("A", 1);
    var_dump ("OPPA!");
  } catch (Exception $e) {
    report_exception ($e, __LINE__);
  }

  try {
    try {
      if (1) {
        throw new Exception ("C", 3);
        var_dump ("OPPA!");
      }
    } catch (Exception $e) {
      report_exception ($e, __LINE__);
      throw new Exception ("B", 2);
    }
  } catch (Exception $e) {
    report_exception ($e, __LINE__);
  }

  try {
    smth();
    var_dump ("OPPA!");
  } catch (Exception $e) {
    report_exception ($e, __LINE__);
  }
}


for ($i = 0; $i < 5000; $i++) {
  test();
}

$x = 0;
test2();
test2();
$x = 1;
test2();
function test2() {
  global $x;
  try {
    require_once '949_throw.php';
    var_dump ("!");
    $y = require '949_throw.php';
    var_dump ($y);
  } catch (Exception $e) {
    report_exception ($e, __LINE__);
  }
}
