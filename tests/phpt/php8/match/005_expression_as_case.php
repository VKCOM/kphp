@ok php8
<?php

function f() {
  return 20;
}

$a = 20;

var_dump(match ($a) {
  f() => "ok",
  default => "oops, default",
});
