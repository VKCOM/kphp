@ok
<?php

class MyException extends Exception {}

try {
  throw new Exception();
} catch (Throwable $e) {
  var_dump(__LINE__, basename($e->getFile()), get_class($e));
}

class Foo {
  public static function f() {
    try {
      throw new MyException();
    } catch (Throwable $e) {
      var_dump(__LINE__, $e->getLine(), get_class($e));
    }
  }
}

function f() {
  try {
    throw new Exception();
  } catch (MyException $e) {
    var_dump([__LINE__ => $e->getLine()]);
  } catch (\Throwable $e) {
    var_dump([__LINE__ => $e->getLine()]);
  }
}

Foo::f();
f();
