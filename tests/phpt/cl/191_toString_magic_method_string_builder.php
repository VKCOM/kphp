@ok
<?php

class A {
  public function __toString() {
    return "hello";
  }

  public function getCopy(): A {
    return $this;
  }

  public function getArray(): array {
    return [$this];
  }
}

function demo(A $a) {
  echo "$a and $a";
  echo "{$a->getCopy()} {$a->getArray()[0]}";
}

demo(new A);
