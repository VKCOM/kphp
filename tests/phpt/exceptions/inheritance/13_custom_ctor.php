@ok
<?php

// Test that we can override the constructor in the custom Exception,
// call the parent constructor and initialize the custom exception itself.

class MyException extends Exception {
  public $tag = '';

  public function __construct(string $tag) {
    parent::__construct('message ' . $tag);
    $this->tag = $tag;
  }
}

$e = new MyException('TAG');
var_dump($e->tag);
f($e);

function f(Exception $e) {
  var_dump($e->getLine());
  var_dump(basename($e->getFile()));
  var_dump($e->getMessage());
  var_dump($e->getCode());
}
