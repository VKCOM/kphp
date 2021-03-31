<?php

/** @kphp-strict-types-enable */
declare(strict_types=1);

function test_regexp_string(string $re) {
  var_dump(preg_match($re, '123') > 0);
  var_dump(preg_match_all($re, '123') > 0);
}

/** @param mixed $re */
function test_regexp_mixed($re) {
  var_dump(preg_match($re, '123') > 0);
  var_dump(preg_match_all($re, '123') > 0);
}

test_regexp_string('/\d/');
test_regexp_mixed('/\d/');
