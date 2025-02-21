@ok
<?php

// Test multiple interfaces for errors.

interface StringInterface {
  public function toString(): string;
}

interface PrettyStringInterface {
  public function toPrettyString(): string;
}

class MyError extends Error implements StringInterface, PrettyStringInterface {
  public function toString(): string {
    $relative = basename($this->file);
    return "$relative:$this->line: $this->message";
  }

  public function toPrettyString(): string {
    $relative = basename($this->file);
    return "so pretty: $relative:$this->line: $this->message";
  }
}

function dump1(StringInterface $s) {
  var_dump($s->toString());
}

function dump2(PrettyStringInterface $s) {
  var_dump($s->toPrettyString());
}

function test(Error $e) {
  try {
    throw $e;
  } catch (MyError $e2) {
    dump1($e2);
    dump2($e2);
  }
}

$e = new MyError('test');
test($e);
