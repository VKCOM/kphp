@ok k2_skip
<?php

// https://github.com/VKCOM/kphp/issues/145

function test_negative_offset1() {
  var_dump(preg_match('/\d+/', 'hello world', $m, 0, -1));
  var_dump($m);
  var_dump(preg_last_error());
}

function test_negative_offset2($flags) {
  var_dump(preg_match('/world/', 'hello world', $m, $flags, -1));
  var_dump($m);
  var_dump(preg_last_error());

  var_dump(preg_match('/world/', 'hello world', $m2, $flags, -6));
  var_dump($m2);
  var_dump(preg_last_error());

  var_dump(preg_match('/hello/', 'hello world', $m3, $flags, -6));
  var_dump($m3);
  var_dump(preg_last_error());

  // too negative
  var_dump(preg_match('/world/', 'hello world', $m4, $flags, -500));
  var_dump($m4);
  var_dump(preg_last_error());
}

test_negative_offset1();
test_negative_offset2(0);
test_negative_offset2(PREG_OFFSET_CAPTURE);
