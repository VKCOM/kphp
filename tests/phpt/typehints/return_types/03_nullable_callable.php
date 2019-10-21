@ok
<?php
function factory($ret_null): ?callable {
  if ($ret_null) {
    return null;
  }
  return function () {
    echo "Done\n";
  };
}

$f = factory(true);
if (!$f) {
  echo "F is NULL\n";
}

$f = factory(false);
$f();

