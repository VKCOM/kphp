@ok k2_skip
<?php

function test() {
  try {
    throw new Exception('the message', 103);
  } catch (Exception $e) {
    var_dump([
      $e->getMessage(),
      $e->getCode(),
      basename($e->getFile()),
      $e->getLine(),
      count($e->getTrace()) !== 0,
      strlen($e->getTraceAsString()) !== 0,
      get_class($e),
    ]);
  }
}

test();
