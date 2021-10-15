@ok php8
<?php

class A {
  public function foo(): string {
    return "hello";
  }

  public function bar(): never {
    throw new UnexpectedValueException('parent');
  }

  public function someReturningStaticMethod(): static {
    return $this;
  }
}

class B extends A {
  public function foo(): never {
    throw new UnexpectedValueException('bad');
  }

  public function bar(): never {
    throw new UnexpectedValueException('child');
  }

  public function someReturningStaticMethod(): never {
    throw new UnexpectedValueException('child');
  }
}

try {
  (new B)->foo();
} catch (UnexpectedValueException $e) {
  // do nothing
}

try {
  (new B)->bar();
} catch (UnexpectedValueException $e) {
  // do nothing
}

try {
  (new B)->someReturningStaticMethod();
} catch (UnexpectedValueException $e) {
  // do nothing
}

echo "OK!", PHP_EOL;
