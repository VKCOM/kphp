@kphp_should_fail
/Different @var for \$id exist/
/assign int to \$id, modifying a function argument/
<?php

// doesn't work, because such phpdoc prevents splitting, as param $id is used for reading after it
function demo(string $id) {
  /** @var int $id */
  $id = (int)$id;
  echo $id, "\n";
}

demo('3');
