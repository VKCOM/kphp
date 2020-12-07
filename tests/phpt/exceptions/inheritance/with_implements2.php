@ok
<?php

// Test multiple interfaces for exceptions.

interface StringInterface {
  public function toString(): string;
}

interface PrettyStringInterface {
  public function toPrettyString(): string;
}

class MyException extends Exception implements StringInterface, PrettyStringInterface {
  public function toString(): string {
    return "$this->file:$this->line: $this->message";
  }

  public function toPrettyString(): string {
    return "so pretty: $this->file:$this->line: $this->message";
  }
}

function dump1(StringInterface $s) {
  var_dump($s->toString());
}

function dump2(PrettyStringInterface $s) {
  var_dump($s->toPrettyString());
}

function test(Exception $e) {
  try {
    throw $e;
  } catch (MyException $e2) {
    dump1($e2);
    dump2($e2);
  }
}

$e = new MyException('test');
test($e);
