@kphp_should_fail
/Do not use @var for arguments, use @param or type declaration/
<?php

// doesn't work, because such phpdoc prevents splitting, as param $id is used for reading after it
function demo(int $id) {
  /** @var int $id */
  $id = (int)$id;
  echo $id, "\n";
}

demo('3');
