@ok
<?php

function test_curl_multi_empty_exec() {
  $mh = curl_multi_init();

  $still_running = 0;
  var_dump(curl_multi_exec($mh, $still_running));
  var_dump($still_running);

  curl_multi_close($mh);
}

test_curl_multi_empty_exec();
