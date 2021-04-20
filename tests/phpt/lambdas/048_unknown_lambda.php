@ok
<?php

class AA {
  /** @param callable(int): bool $f */
  function foo($f) {
     $f(2);
  }
}

new AA;
