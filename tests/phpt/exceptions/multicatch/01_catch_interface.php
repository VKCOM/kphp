@ok
<?php

// Test catching of interfaces.

interface CustomExceptionInterface {
  public function describe(): string;
}

class BaseException extends Exception implements CustomExceptionInterface {
  public function describe(): string {
    $relative = basename($this->file);
    return "$relative:$this->line: $this->message";
  }
}

class DerivedException extends BaseException {}

function test1() {
  try {
    throw new Exception();
  } catch (CustomExceptionInterface $e) {
    var_dump([__LINE__ => $e->describe()]); // should not be executed
  }
}

function test2() {
  try {
    throw new BaseException();
  } catch (CustomExceptionInterface $e) {
    var_dump([__LINE__ => $e->describe()]);
  } catch (BaseException $e2) {
    var_dump([__LINE__ => 'should not catch']);
  }
  var_dump(__LINE__);

  try {
    throw new DerivedException();
  } catch (CustomExceptionInterface $e) {
    var_dump([__LINE__ => $e->describe()]);
  } catch (DerivedException $e2) {
    var_dump([__LINE__ => 'should not catch']);
  }
  var_dump(__LINE__);
}

function test3() {
  try {
    throw new BaseException();
  } catch (BaseException $e) {
    var_dump([__LINE__ => $e->getLine()]);
  } catch (CustomExceptionInterface $e2) {
    var_dump([__LINE__ => 'should not catch']);
  }
  var_dump(__LINE__);
}

try {
  test1();
} catch (Exception $e) {
  var_dump([__LINE__ => $e->getLine()]);
}

test2();
test3();
