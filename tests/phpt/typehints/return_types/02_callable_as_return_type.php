@ok
<?php
function factory(): callable {
  return function () {
    echo "Done";
  };
}

$foo = factory();
$foo();

