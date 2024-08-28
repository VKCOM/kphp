@ok k2_skip
<?php

function test() {
  try {
    throw new Exception('the message', 103);
  } catch (Throwable $e) {
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

  try {
    throw new Error('the message', 103);
  } catch (Throwable $e2) {
    var_dump([
      $e2->getMessage(),
      $e2->getCode(),
      basename($e2->getFile()),
      $e2->getLine(),
      count($e2->getTrace()) !== 0,
      strlen($e2->getTraceAsString()) !== 0,
      get_class($e2),
    ]);
  }
}

test();
