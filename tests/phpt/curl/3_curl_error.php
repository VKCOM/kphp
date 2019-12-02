@ok
<?php

function test_no_errors() {
  $c = curl_init();

  var_dump(curl_error($c));
  var_dump(curl_errno($c));

  curl_close($c);
}



test_no_errors();
