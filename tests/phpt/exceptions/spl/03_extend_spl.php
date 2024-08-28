@ok k2_skip
KPHP_REQUIRE_FUNCTIONS_TYPING=1
<?php

interface CustomExceptionInterface {
  public function getClass(): string;
}

class HypeOverflowException extends OverflowException implements CustomExceptionInterface {
  public $secret = 0;

  public function __construct(string $message) {
    parent::__construct($message);
    $this->code = 5023;
    $this->secret = 40;
  }

  public function getClass(): string {
    return get_class($this);
  }
}

try {
  try {
    throw new HypeOverflowException('test1');
  } catch (Throwable $throwable) {
    var_dump([__LINE__ => $throwable->getCode()]);
    var_dump([__LINE__ => $throwable->getMessage()]);
    throw $throwable;
  }
} catch (HypeOverflowException $e) {
  var_dump([__LINE__ => $e->getCode()]);
  var_dump([__LINE__ => $e->getMessage()]);
  var_dump([__LINE__ => $e->getClass()]);
  var_dump([__LINE__ => $e->secret]);
}

try {
  throw new HypeOverflowException('test2');
} catch (Exception $e2) {
  var_dump([__LINE__ => $e2->getCode()]);
  var_dump([__LINE__ => $e2->getMessage()]);
}

try {
  throw new HypeOverflowException('test2');
} catch (CustomExceptionInterface $e3) {
  var_dump([__LINE__ => $e3->getClass()]);
}
