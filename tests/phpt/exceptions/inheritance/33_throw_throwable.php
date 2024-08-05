@ok k2_skip
<?php

interface CustomException {
  public function toString(): string;
}

class BaseException extends Exception implements CustomException {
  public function toString(): string {
    $relative = basename($this->file);
    return "BaseException $relative:$this->line: $this->message";
  }
}

class DerivedException extends Exception {
  public function toString(): string {
    $relative = basename($this->file);
    return "DerivedException $relative:$this->line: $this->message";
  }
}

function do_throw(Throwable $e) {
  try {
    throw $e;
  } catch (Throwable $e2) {
    var_dump([__LINE__ => $e2->getMessage()]);
    var_dump([__LINE__ => $e2->getLine()]);
    var_dump([__LINE__ => get_class($e2)]);
  }
  if ($e instanceof CustomException) {
    var_dump([__LINE__ => $e->toString()]);
  }
}

do_throw(new Exception('builtin exception'));
do_throw(new BaseException('custom base exception'));
do_throw(new DerivedException('custom derived exception'));
