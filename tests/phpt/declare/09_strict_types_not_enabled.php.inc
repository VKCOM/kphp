<?php

declare(strict_types=1);

function f($cond) {
  if ($cond) {
    var_dump(strcmp(40, 23));
  }
}

f(false);
