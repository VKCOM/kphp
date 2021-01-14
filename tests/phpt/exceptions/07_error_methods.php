@ok
<?php

function test() {
  try {
    throw new Error('the message', 103);
  } catch (Error $e) {
    var_dump([
      $e->getMessage(),
      $e->getCode(),
      $e->getFile(),
      $e->getLine(),
      count($e->getTrace()) !== 0,
      strlen($e->getTraceAsString()) !== 0,
      get_class($e),
    ]);
  }
}

test();
