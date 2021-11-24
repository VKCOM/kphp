@ok
<?php

// PHP defines 4 protected props for the Exception class:
// $file, $line, $code and $message.
//
// This test checks that they work as ordinary protected props
// and can be accessed/modified from the derived exceptions.

class MyException extends Exception {
  public function getLocation(): string {
    return basename($this->file) . ':' . $this->line;
  }

  public function setLine(int $line): void {
    $this->line = $line;
  }
}

$e = new MyException();
var_dump($e->getLocation());
$e->setLine(9000);
var_dump($e->getLocation());
