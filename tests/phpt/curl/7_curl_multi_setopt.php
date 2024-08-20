@ok k2_skip
<?php

function test_curl_multi_setopt() {
  $mh = curl_multi_init();

  var_dump(curl_multi_setopt($mh, CURLMOPT_PIPELINING, CURLPIPE_HTTP1));
  var_dump(curl_multi_setopt($mh, CURLMOPT_MAXCONNECTS, 10));
  var_dump(curl_multi_setopt($mh, CURLMOPT_CHUNK_LENGTH_PENALTY_SIZE, 111));
  var_dump(curl_multi_setopt($mh, CURLMOPT_CONTENT_LENGTH_PENALTY_SIZE, 222));
  var_dump(curl_multi_setopt($mh, CURLMOPT_MAX_HOST_CONNECTIONS, 123));
  var_dump(curl_multi_setopt($mh, CURLMOPT_MAX_PIPELINE_LENGTH, 777));
  var_dump(curl_multi_setopt($mh, CURLMOPT_MAX_TOTAL_CONNECTIONS, 87));

  // bad option
  var_dump(curl_multi_setopt($mh, 98491981, 87));

  curl_multi_close($mh);
}

test_curl_multi_setopt();
