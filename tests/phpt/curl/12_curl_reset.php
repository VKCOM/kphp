@ok k2_skip
<?php

function test_reset() {
  $c = curl_init("http://example.com/");
  var_dump(curl_setopt($c, CURLOPT_PRIVATE, "Hello!"));

  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));
  var_dump(curl_getinfo($c, CURLINFO_EFFECTIVE_URL));

  curl_reset($c);
  var_dump(curl_getinfo($c, CURLINFO_EFFECTIVE_URL));
  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));

  curl_close($c);
}

test_reset();
