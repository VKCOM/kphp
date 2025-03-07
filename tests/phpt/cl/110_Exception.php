@ok
<?php
require_once 'kphp_tester_include.php';

use Classes\A;
use Classes\B;

function report_exception(Exception $exception) {
  echo "ex ", $exception->getMessage(), "\n";
}

try {
  $a = new A();
  throw new Exception('msg');
  $a->printA();
} catch (Exception $ex) {
  echo "ex ", $ex->getMessage(), "\n";
}

try {
  $a = new A();
} catch (Exception $ex) {
  echo "ex ", $ex->getMessage(), "\n";
}

try {
  throw new Exception(12);
} catch (Exception $ex) {
  report_exception($ex);
}
