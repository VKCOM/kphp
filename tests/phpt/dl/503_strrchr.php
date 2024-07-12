@ok k2_skip
<?php

function test_empty_strings() {
  var_dump(strrchr("", ""));
  var_dump(strrchr("hello world", ""));
  var_dump(strrchr("", "hello world"));
}

function test_not_found() {
  var_dump(strrchr("hello world", "x"));
  var_dump(strrchr("hello world", "\0"));
}

function test_several_occurrence() {
  var_dump(strrchr("hello world", "l"));
  var_dump(strrchr("hello world", "o"));
}

function test_one_occurrence() {
  var_dump(strrchr("hello world", " "));
  var_dump(strrchr("hello world", "e"));
}

function test_long_needle_not_found() {
  var_dump(strrchr("hello world", "test"));
  var_dump(strrchr("hello world", "not found"));
}

function test_long_needle() {
  var_dump(strrchr("hello world", "write"));
  var_dump(strrchr("hello world", "hi"));
}

test_empty_strings();
test_not_found();
test_several_occurrence();
test_one_occurrence();
test_long_needle_not_found();
test_long_needle();
