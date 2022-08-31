@ok
<?php

function test_preg_errors_constants() {
  var_dump(PREG_NO_ERROR);
  var_dump(PREG_INTERNAL_ERROR);
  var_dump(PREG_BACKTRACK_LIMIT_ERROR);
  var_dump(PREG_RECURSION_LIMIT_ERROR);
  var_dump(PREG_BAD_UTF8_ERROR);
}

function test_preg_errors() {
  var_dump(preg_match('/(foo)(bar)(baz)/', 'foobarbaz', $matches));
  var_dump(preg_last_error() == PREG_NO_ERROR);

  var_dump(preg_match('/\d+/', 'hello world', $m, 0, 100));
  var_dump(preg_last_error() == PREG_INTERNAL_ERROR);

  var_dump(preg_match('/(?:\D+|<\d+>)*[!?]/', 'foobar foobar foobar'));
  var_dump(preg_last_error() == PREG_BACKTRACK_LIMIT_ERROR);

  var_dump(preg_split("/a/u", "a\xff"));
  var_dump(preg_last_error() == PREG_BAD_UTF8_ERROR);
}

test_preg_errors_constants();
test_preg_errors();
