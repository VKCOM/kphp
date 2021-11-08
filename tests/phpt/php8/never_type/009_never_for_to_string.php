@kphp_should_fail php8
/Using result of never function B::__toString/
<?php

class A {
  public function __toString(): string {
    return "hello";
  }
}

class B extends A {
  public function __toString(): never {
    throw new \Exception('not supported');
  }
}

try {
  echo (string) (new B());
} catch (Exception $e) {
  // do nothing
}

echo "done";
