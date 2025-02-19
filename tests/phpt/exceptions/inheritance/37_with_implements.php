@ok
<?php

// Test classes that both extend Exception and implement some PHP interface.

interface StringInterface {
  public function toString(): string;
}

class MyException extends Exception implements StringInterface {
  public function toString(): string {
    $relative = basename($this->file);
    return "$relative:$this->line: $this->message";
  }
}

function dump(StringInterface $s) {
  var_dump($s->toString());
}

function test() {
  $e = new MyException('test');
  try {
    throw $e;
  } catch (MyException $e2) {
    dump($e2);
  }
}

test();
