@ok
<?php

class A {
  function __toString() {
    return "hello";
  }
}

function demo(A $a) {
  echo "$a and $a";
}

demo(new A);
