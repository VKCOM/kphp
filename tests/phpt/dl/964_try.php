@ok k2_skip
<?php
/**
 * @param string $desc
 * @return Exception
 */
function getException($desc) {
  return new Exception ($desc);
}

function processException (Exception $e) {
  echo 'message: ',  $e->getMessage(), "\n";
  echo "code = ", $e->getCode(), "\n";
  echo "file = ", basename($e->getFile()), "\n";
  echo "line = ", $e->getLine(), "\n"; 
//  var_dump (count ($e->getTrace()));
  var_dump (@count ($e->getTraceAsString()));
}

try {
  throw getException ("Test1");
} catch (Exception $e) {
  processException ($e);  
}

/**
 * @param int $x
 * @return float
 */
function inverse($x) {
    if (!$x) {
        throw new Exception('Деление на ноль', 123);
    }
    else return 1/$x;
}

try {
    echo inverse(5) . "\n";
    echo inverse(0) . "\n";
} catch (Exception $e) {
    echo 'Выброшено исключение: ',  $e->getMessage(), "\n";
    echo "code = ", $e->getCode(), "\n";
    echo "file = ", basename($e->getFile()), "\n";
    echo "line = ", $e->getLine(), "\n";
//    var_dump (count ($e->getTrace()));
    var_dump (@count ($e->getTraceAsString()));
    $debug .= 'CODE_'.$e->getCode().' '.$e->getMessage()."\n";
}

// Продолжение выполнения
echo 'Hello World'."\n";

//echo inverse(0) . "\n";

/**
 * @return Exception
 */
function f()  {
  return new Exception ("a", 123);
  throw new Exception ("a", 123);
}

try {
  throw f();        
} catch (Exception $e) {
  processException($e);
}

try {
  inverse(0);
} catch (Exception $e) {
  apiWrapError($e, 10, 'INPUT_FETCH_ERROR');
}

/**
 * @param int|false $error_code
 * @param string $error_description
 */
function apiWrapError(Exception $exception, $error_code = false, $error_description = '') {
  $error_type = "??";
  $error_data = array(
    'code' => $error_code,
    'type' => $error_type,
    'description' => $error_description,
    'debug' => $exception ? ($exception->getMessage())."\n".(@count ($exception->getTraceAsString())) : '',
  );

  $request_params = "???";
  $error_data['request_params'] = $request_params;
  var_dump($error_data);
  exit;
}

