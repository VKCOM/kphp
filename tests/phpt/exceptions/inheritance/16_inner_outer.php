@ok k2_skip
<?php

// Test throwing and catching custom exceptions.

class MyException1 extends \Exception {}
class MyException2 extends \Exception {}
class MyException1derived extends MyException1 {}

function exception_string(Exception $e, string $tag): string {
  return basename($e->getFile()) . ':' . $e->getLine() . ':' . $tag;
}

function test1() {
  // should be caught by inner
  try {
    try {
      throw new MyException1();
    } catch (MyException1 $e1) {
      var_dump(exception_string($e1, 'inner'));
    }
  } catch (Exception $e2) {
    var_dump(exception_string($e2, 'outer'));
  }
}

function test2() {
  // should be caught by inner
  try {
    try {
      throw new MyException2();
    } catch (MyException2 $e1) {
      var_dump(exception_string($e1, 'inner'));
    }
  } catch (Exception $e2) {
    var_dump(exception_string($e2, 'outer'));
  }

  // should be caught by outer: MyException2 extends Exception
  try {
    try {
      throw new MyException2();
    } catch (MyException1 $e1) {
      var_dump(exception_string($e1, 'inner'));
    }
  } catch (Exception $e2) {
    var_dump(exception_string($e2, 'outer'));
  }
}

function test3() {
  // should be caught by inner
  try {
    try {
      throw new MyException1derived();
    } catch (MyException1derived $e1) {
      var_dump(exception_string($e1, 'inner'));
    }
  } catch (Exception $e2) {
    var_dump(exception_string($e2, 'outer'));
  }

  // should be caught by inner: MyException1derived extends MyException1
  try {
    try {
      throw new MyException1derived();
    } catch (MyException1 $e1) {
      var_dump(exception_string($e1, 'inner'));
    }
  } catch (Exception $e2) {
    var_dump(exception_string($e2, 'outer'));
  }

  // should be caught by outer: MyException1derived extends Exception via MyException1
  try {
    try {
      throw new MyException1derived();
    } catch (MyException2 $e1) {
      var_dump(exception_string($e1, 'inner'));
    }
  } catch (Exception $e2) {
    var_dump(exception_string($e2, 'outer'));
  }
}

test1();
test2();
test3();
