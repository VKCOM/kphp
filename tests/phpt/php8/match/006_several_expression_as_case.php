@ok php8
<?php

function f1() {
  return 20;
}

function f2() {
  echo "oops, shouldn't have been called\n";
  return 10;
}

$a = 20;

var_dump(match ($a) {
  f1() => "ok",
  f2() => "oops",
  default => "oops, default",
});
