@ok
<?php

function test_no_error() {
  $mh = curl_multi_init();
  $errno = curl_multi_errno($mh);
  var_dump($errno);
  var_dump(curl_multi_strerror($errno));
  curl_multi_close($mh);
}

test_no_error();
