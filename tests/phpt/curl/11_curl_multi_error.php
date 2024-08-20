@ok k2_skip
<?php

function test_no_error() {
  $mh = curl_multi_init();
  var_dump(curl_multi_errno($mh));
  curl_multi_close($mh);
}

test_no_error();
