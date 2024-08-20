@ok k2_skip
<?php

// Test catching of interfaces with inheritance.

interface BaseCustomExceptionInterface {
  public function isCustomException(): bool;
}

interface CustomExceptionInterface extends BaseCustomExceptionInterface {
  public function describe(): string;
}

class BaseException extends Exception implements CustomExceptionInterface {
  public function describe(): string {
    $relative = basename($this->file);
    return "$relative:$this->line: $this->message";
  }

  public function isCustomException(): bool { return true; }
}

class DerivedException extends BaseException {}

function test1() {
  try {
    throw new BaseException();
  } catch (BaseCustomExceptionInterface $e) {
    var_dump([__LINE__ => $e->isCustomException()]);
  }

  try {
    throw new DerivedException();
  } catch (CustomExceptionInterface $e2) {
    var_dump([__LINE__ => $e2->describe()]);
  }
}

test1();
