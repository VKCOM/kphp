@ok k2_skip
<?php

function test_static_preg_errors() {
  var_dump(preg_match('[ \(\)+-]/', 'simple', $matches));
  var_dump(preg_match('[\]sigsegv', 'simple', $matches));
}

function test_preg_errors_constants() {
  var_dump(PREG_NO_ERROR);
  var_dump(PREG_INTERNAL_ERROR);
  var_dump(PREG_BACKTRACK_LIMIT_ERROR);
  var_dump(PREG_RECURSION_LIMIT_ERROR);
  var_dump(PREG_BAD_UTF8_ERROR);
  var_dump(PREG_BAD_UTF8_OFFSET_ERROR);
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

function test_preg_errors_offset() {
  var_dump(preg_match('~.*~u', "a\xc2\xa9b", $m, 0, 0));
  var_dump($m);
  var_dump(preg_last_error() == PREG_NO_ERROR);

  var_dump(preg_match('~.*~u', "a\xc2\xa9b", $m, 0, 1));
  var_dump($m);
  var_dump(preg_last_error() == PREG_NO_ERROR);

  var_dump(preg_match('~.*~u', "a\xc2\xa9b", $m, 0, 2));
  var_dump($m);
  var_dump(preg_last_error() == PREG_BAD_UTF8_OFFSET_ERROR);

  var_dump(preg_match('~.*~u', "a\xc2\xa9b", $m, 0, 3));
  var_dump($m);
  var_dump(preg_last_error() == PREG_NO_ERROR);

  var_dump(preg_match('~.*~u', "a\xc2\xa9b", $m, 0, 4));
  var_dump($m);
  var_dump(preg_last_error() == PREG_NO_ERROR);

  var_dump(preg_match('~.*~u', "a\xc2\xa9b", $m, 0, 5));
  var_dump($m);
  var_dump(preg_last_error() == PREG_INTERNAL_ERROR);

  var_dump(preg_match('~.*~u', "a\xa9\xc2b", $m, 0, 0));
  var_dump($m);
  var_dump(preg_last_error() == PREG_BAD_UTF8_ERROR);

  var_dump(preg_match('~.*~u', "a\xa9\xc2b", $m, 0, 2));
  var_dump($m);
  var_dump(preg_last_error() == PREG_BAD_UTF8_ERROR);

  var_dump(preg_match('~.*~u', "\xa9\xc2b", $m, 0, 0));
  var_dump($m);
  var_dump(preg_last_error() == PREG_BAD_UTF8_ERROR);

  var_dump(preg_match('~.*~u', "\xa9", $m, 0, 0));
  var_dump($m);
  var_dump(preg_last_error() == PREG_BAD_UTF8_ERROR);
}

test_static_preg_errors();
test_preg_errors_constants();
test_preg_errors();
test_preg_errors_offset();
