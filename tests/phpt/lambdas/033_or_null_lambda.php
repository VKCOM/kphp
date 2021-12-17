@ok
<?php

$id = null;
$id = function ($x) { return $x; };
if ($id) {
    var_dump("id");
}

/** @param callable():void $callback */
function callMe($callback) {
  /** @var callable():void $x */
  $x = $callback;
  $x = function() { echo "asdf"; };
  $x();
}

callMe(function() { echo "Hello"; });
callMe(null);
