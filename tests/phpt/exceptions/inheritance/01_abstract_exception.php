@ok
<?php

interface SecretInterface {
  public function getSecret(): string;
}

abstract class BaseException extends Exception implements SecretInterface {
  abstract public function getSecret(): string;
}

class DerivedException1 extends BaseException {
  public function getSecret(): string { return 'a'; }
}

class DerivedException2 extends BaseException {
  public function getSecret(): string { return 'b'; }
}

class DerivedException3 extends BaseException {
  public function getSecret(): string { return 'c'; }
}

function test() {
  $e = new DerivedException3();
  $f1 = function() use ($e) {
    try {
      throw $e;
    } catch (DerivedException1 $e2) {
      var_dump([__LINE__ => 'should not catch']);
    }
  };
  $f2 = function(Exception $e) {
    try {
      throw $e;
    } catch (DerivedException2 $e2) {
      var_dump([__LINE__ => 'should not catch']);
    }
  };

  for ($i = 0; $i < 5; $i++) {
    try {
      $f1();
    } catch (BaseException $e2) {
      var_dump([__LINE__ => $e2->getLine()]);
      var_dump([__LINE__ => $e2->getSecret()]);
    }
  }

  try {
    $f2(new DerivedException1());
  } catch (BaseException $e3) {
    var_dump([__LINE__ => $e3->getLine()]);
    var_dump([__LINE__ => $e3->getSecret()]);
  }
}

test();
