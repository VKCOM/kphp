@ok
<?php

interface CustomError {
  public function toString(): string;
}

class BaseError extends Error implements CustomError {
  public function toString(): string {
    $relative = basename($this->file);
    return "BaseError $relative:$this->line: $this->message";
  }
}

class DerivedError extends Error {
  public function toString(): string {
    $relative = basename($this->file);
    return "DerivedError $relative:$this->line: $this->message";
  }
}

function do_throw(Error $e) {
  try {
    throw $e;
  } catch (Error $e2) {
    var_dump([__LINE__ => $e2->getMessage()]);
    var_dump([__LINE__ => $e2->getLine()]);
    var_dump([__LINE__ => get_class($e2)]);
  }
  if ($e instanceof CustomError) {
    var_dump([__LINE__ => $e->toString()]);
  }
}

do_throw(new Error('builtin error'));
do_throw(new BaseError('custom base error'));
do_throw(new DerivedError('custom derived error'));
