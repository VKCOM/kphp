@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

/**
 * @param int $x
 * @param int $p
 * @return int
 */
function my_pow ($x, $p = 5) {
  if ($p == 0) {
    if ($x == 5) {
      throw new Exception("5 is not supported as first parameter of my_pow", 239);
    }
    return 1;
  }
  if ($p == 2 || $p == 1) {
    $id = fork (my_pow ($x, $p - 1));
    $tmp = wait_synchronously($id);
  } else {
    $tmp = my_pow ($x, $p - 1);
  }
  return $tmp * $x;
}

function processException (Exception $e) {
  echo 'message: ',  $e->getMessage(), "\n";
  echo "code = ", $e->getCode(), "\n";
  echo "file = ", basename($e->getFile()), "\n";
  echo "line = ", $e->getLine(), "\n";
}

echo "-----------<stage 1>-----------\n";

try {
  var_dump (my_pow (5, 2)." != 5 ^ 2");
} catch (Exception $e) {
  processException ($e);  
}

echo "-----------<stage 2>-----------\n";

try {
  var_dump (my_pow (4, 3)." = 4 ^ 3");
} catch (Exception $e) {
  processException ($e);  
}

echo "-----------<stage 3>-----------\n";

try {
  $x = fork (my_pow (5, 7));
  echo "OK!\n";
} catch (Exception $e) {
#ifndef KPHP
  $x = fork (0);
  echo "OK!\n";
  $exception_from_fork = $e;
  if (false)
#endif
  {
    processException ($e);
    echo "ERROR!\n";
  }

}

echo "-----------<stage 6>-----------\n";

try {
  #ifndef KPHP
    throw $exception_from_fork;
  #endif
  var_dump (wait_synchronously($x));
  echo "ERROR!\n";
} catch (Exception $e) {
  processException ($e);  
  echo "OK!\n";
}

echo "-----------<stage 7>-----------\n";

try {
  $x = fork (my_pow (4));
  var_dump (wait($x));
  var_dump (wait($x));
  echo "OK!\n";
} catch (Exception $e) {
  processException ($e);  
  echo "ERROR!\n";
}

