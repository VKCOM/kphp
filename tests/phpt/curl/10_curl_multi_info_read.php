@ok
<?php

function test_curl_multi_info_read_empty() {
  $mh = curl_multi_init();

  var_dump(curl_multi_info_read($mh));

  curl_multi_close($mh);
}

function test_curl_multi_info_read_empty_with_msgs_in_queue() {
  $mh = curl_multi_init();

  $msgs_in_queue = 0;
  var_dump(curl_multi_info_read($mh, $msgs_in_queue));
  var_dump($msgs_in_queue);

  curl_multi_close($mh);
}

function test_curl_multi_info_read() {
  $mh = curl_multi_init();

  $ch = curl_init("http://example.com");
  var_dump(curl_multi_add_handle($mh, $ch));
  var_dump(curl_multi_info_read($mh));

  curl_multi_close($mh);
}

function test_curl_multi_info_read_with_msgs_in_queue() {
  $mh = curl_multi_init();

  $ch = curl_init("http://example.com");
  var_dump(curl_multi_add_handle($mh, $ch));

  $msgs_in_queue = 0;
  var_dump(curl_multi_info_read($mh, $msgs_in_queue));
  var_dump($msgs_in_queue);

  curl_multi_close($mh);
}

test_curl_multi_info_read_empty();
test_curl_multi_info_read_empty_with_msgs_in_queue();
test_curl_multi_info_read();
test_curl_multi_info_read_with_msgs_in_queue();
