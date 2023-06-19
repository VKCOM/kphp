@ok php8
<?php

$a = 10;

var_dump(match ($a) {
  "10" => "oops, type casting",
  default => "ok",
});
