@ok
<?php

function getCode(\Exception $e): string {
  return (string)$e->getCode();
}

function test1() {
  try {
    throw new Exception('hello world !');
  } catch (\Exception $e) {
    list(, $message) = explode(' ', $e->getMessage(), 2);
    return $message;
  }
  return '?';
}

function test2() {
  try {
    throw new Exception('', 50);
  } catch (\Exception $e) {
    var_dump([__LINE__ => $e->getCode()]);
    $code = getCode($e);
    return $code;
  }
  return '?';
}

var_dump(test1());
var_dump(test2());
