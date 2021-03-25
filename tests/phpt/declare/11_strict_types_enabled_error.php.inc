<?php

declare(strict_types=1);

class A {
  public $x = 1;
}

function test_regexp_crap($re) {
  var_dump(preg_match($re, '123') > 0);
  var_dump(preg_match_all($re, '123') > 0);
}

test_regexp_crap(tuple(1));
