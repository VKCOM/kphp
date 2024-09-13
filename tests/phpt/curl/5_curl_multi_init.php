@ok k2_skip
<?php

function test_curl_multi_init() {
  $mh = curl_multi_init();
  curl_multi_close($mh);
}

function test_curl_multi_init_no_close() {
  $mh = curl_multi_init();
}

test_curl_multi_init();
test_curl_multi_init_no_close();
