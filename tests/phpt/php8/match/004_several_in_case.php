@ok php8
<?php

$a = 20;

var_dump(match ($a) {
  10, 20, 30 => "ok",
  default => "oops, default",
});
