@ok k2_skip
<?php

// See issue #201

function test_simple() {
  var_dump(preg_match_all('/(a)?/', 'a', $matches, PREG_PATTERN_ORDER | PREG_OFFSET_CAPTURE));
  var_dump($matches);

  var_dump(preg_match_all('/(.)x/', 'zxyx', $match, PREG_PATTERN_ORDER | PREG_OFFSET_CAPTURE));
  var_dump($match);

  var_dump(preg_match_all('/(.)x/', 'zxyx', $match, PREG_SET_ORDER | PREG_OFFSET_CAPTURE));
  var_dump($match);
}

function test_named_capture($extra_flags) {
  preg_match_all('/(?<a>4)?(?<b>2)?\d/', '23456', $matches, $extra_flags);
  var_dump($matches);

  preg_match_all('/(?<a>4)?(?<b>2)?\d/', '23456', $matches, $extra_flags | PREG_OFFSET_CAPTURE);
  var_dump($matches);

  preg_match_all('/(?<a>4)?(?<b>2)?\d/', '123456', $matches, $extra_flags);
  var_dump($matches);

  preg_match_all('/(?<a>4)?(?<b>2)?\d/', '123456', $matches, $extra_flags | PREG_OFFSET_CAPTURE);
  var_dump($matches);
}

function test_unnamed_capture($extra_flags) {
  preg_match_all('/(4)?(2)?\d/', '23456', $matches, $extra_flags);
  var_dump($matches);

  preg_match_all('/(4)?(2)?\d/', '23456', $matches, $extra_flags | PREG_OFFSET_CAPTURE);
  var_dump($matches);

  preg_match_all('/(4)?(2)?\d/', '123456', $matches, $extra_flags);
  var_dump($matches);

  preg_match_all('/(4)?(2)?\d/', '123456', $matches, $extra_flags | PREG_OFFSET_CAPTURE);
  var_dump($matches);
}

function test_utf8() {
  $message = 'Der ist ein Süßwasserpool Süsswasserpool ... verschiedene Wassersportmöglichkeiten bei ...';

  $pattern = '/\bwasser/iu';
  preg_match_all($pattern, $message, $match, PREG_PATTERN_ORDER | PREG_OFFSET_CAPTURE);
  var_dump($match);

  $pattern = '/[^\w]wasser/iu';
  preg_match_all($pattern, $message, $match, PREG_PATTERN_ORDER | PREG_OFFSET_CAPTURE);
  var_dump($match);
}

test_simple();

test_named_capture(0);
test_named_capture(PREG_PATTERN_ORDER);
test_named_capture(PREG_SET_ORDER);

test_unnamed_capture(0);
test_unnamed_capture(PREG_PATTERN_ORDER);
test_unnamed_capture(PREG_SET_ORDER);

test_utf8();
