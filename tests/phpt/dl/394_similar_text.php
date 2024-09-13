@ok k2_skip
<?php

function do_similar_text_test(string $hint, string $first, string $second) {
  echo "test ", $hint, "\n";
  echo "no param >> ", similar_text($first, $second), "\n";
  $p = 0.0;
  echo "with param >> ", similar_text($first, $second, $p), " ";
  echo " >> ", $p, "\n\n";
}

function test_empty() {
  do_similar_text_test("both empty", "", "");
  do_similar_text_test("first empty", "heioaskdpweuhfiwefwe", "");
  do_similar_text_test("second empty", "", "ldj duifd hufweuyiweubfe");
}

function test_different() {
  do_similar_text_test("test different 1", "hello", "ward");
  do_similar_text_test("test different 2", "abcdefg", "hijklmnop");
  do_similar_text_test("test different 3", "aaaaaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbbbbb");
  do_similar_text_test("test different 4", "aaaaaaaaaaaxxxxxxxxxxxxxx", "0000000000bbbbbbbbbb");
}


function test_similar() {
  do_similar_text_test("test same 1", "foo", "woo");
  do_similar_text_test("test same 2", "hellow", "world");
  do_similar_text_test("test same 3", "aaaaaaaaaaaaaaaaaaaaa", "aaaaaaaaaaaaaaaa");
  do_similar_text_test("test same 4", "aaaaaaaaaaaxxxxxxxxxxxxxx", "xxxxxxxxxxxbbbbbbbbbb");
  do_similar_text_test("test same 5", "test test test test test", "    test    test   ");
}

function test_other() {
  do_similar_text_test("test other 1", "a", "a");
  do_similar_text_test("test other 2", "ab", "ac");
  do_similar_text_test("test other 3", "abcbcbcbcbc", "acccbcbababa");
  do_similar_text_test("test other 4", "ggaa", "aagg");
  do_similar_text_test("test other 5", "hello world", "world hello");
  do_similar_text_test("test other 5", "zxccxzzxccxzzxccxz", "xczzcxxzccxzxczzxc");
}

test_empty();
test_different();
test_similar();
test_other();
