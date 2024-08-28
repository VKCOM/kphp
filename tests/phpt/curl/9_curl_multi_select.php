@ok k2_skip
<?php

function test_curl_multi_select_empty() {
  $mh = curl_multi_init();

  var_dump(curl_multi_select($mh));

  curl_multi_close($mh);
}

test_curl_multi_select_empty();
