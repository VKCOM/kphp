@ok php8
<?php

$a = "hello";

var_dump(match ($a) {
  10 => "oops, error",
  "hello" => "ok",
  default => "oops, default",
});
