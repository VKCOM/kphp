@ok
<?php

function test_curl_multi_add_remove_handle() {
  $mh = curl_multi_init();

  $ch = curl_init("http://example.com");
  var_dump(curl_multi_add_handle($mh, $ch));
  var_dump(curl_multi_remove_handle($mh, $ch));
  curl_close($ch);

  curl_multi_close($mh);
}

function test_curl_multi_add_no_remove_handle() {
  $mh = curl_multi_init();

  $ch = curl_init("http://example.com");
  var_dump(curl_multi_add_handle($mh, $ch));
  curl_close($ch);

  curl_multi_close($mh);
}

function test_curl_multi_add_handle_and_close_curl() {
  $mh = curl_multi_init();

  $ch = curl_init("http://example.com");
  var_dump(curl_multi_add_handle($mh, $ch));

  curl_close($ch);
  curl_multi_close($mh);
}

function test_curl_multi_add_handle_and_close_multi() {
  $mh = curl_multi_init();

  $ch = curl_init("http://example.com");
  var_dump(curl_multi_add_handle($mh, $ch));

  curl_multi_close($mh);
  curl_close($ch);
}

function test_curl_multi_add_handle_no_close_no_remove() {
  $mh = curl_multi_init();

  $ch = curl_init("http://example.com");
  var_dump(curl_multi_add_handle($mh, $ch));
}

function test_curl_multi_remove_without_add() {
  $mh = curl_multi_init();

  $ch = curl_init("http://example.com");
  var_dump(curl_multi_remove_handle($mh, $ch));
}

test_curl_multi_add_remove_handle();
test_curl_multi_add_no_remove_handle();
test_curl_multi_add_handle_and_close_curl();
test_curl_multi_add_handle_and_close_multi();
test_curl_multi_add_handle_no_close_no_remove();
test_curl_multi_remove_without_add();
