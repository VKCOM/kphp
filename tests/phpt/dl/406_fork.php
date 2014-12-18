@ok no_php
<?php
function my_pow ($x, $p = 5) {
  if ($p == 0) {
    if ($x == 5) {
      throw new Exception("5 is not supported as first parameter of my_pow", 239);
    }
    return 1;
  }
  if ($p == 2 || $p == 1) {
    $id = fork (my_pow ($x, $p - 1));
    if (!wait_synchronously ($id)) {
      echo "ERROR!\n";
    }
    $tmp = wait_result ($id);
  } else {
    $tmp = my_pow ($x, $p - 1);
  }
  return $tmp * $x;
}

function processException ($e) {
  echo 'message: ',  $e->getMessage(), "\n";
  echo "code = ", $e->getCode(), "\n";
  echo "file = ", $e->getFile(), "\n";
  echo "line = ", $e->getLine(), "\n"; 
//  var_dump ($e->getTrace());
//  var_dump (@count ($e->getTraceAsString()));
}

try {
  var_dump (my_pow (5, 2)." != 5 ^ 2");
} catch (Exception $e) {
  processException ($e);  
}

try {
  var_dump (my_pow (4, 3)." = 4 ^ 3");
} catch (Exception $e) {
  processException ($e);  
}

try {
  $x = fork (my_pow (5, 7));
  echo "OK!\n";
} catch (Exception $e) {
  processException ($e);  
  echo "ERROR!\n";
}

try {
  var_dump (wait ($x));
  echo "OK!\n";
} catch (Exception $e) {
  processException ($e);  
  echo "ERROR!\n";
}

try {
  var_dump (wait_synchronously ($x));
  echo "OK!\n";
} catch (Exception $e) {
  processException ($e);  
  echo "ERROR!\n";
}

try {
  var_dump (wait_result ($x));
  echo "ERROR!\n";
} catch (Exception $e) {
  processException ($e);  
  echo "OK!\n";
}

try {
  $x = fork (my_pow (4));
  var_dump (wait_result ($x));
  var_dump (wait_result ($x));
  echo "OK!\n";
} catch (Exception $e) {
  processException ($e);  
  echo "ERROR!\n";
}

