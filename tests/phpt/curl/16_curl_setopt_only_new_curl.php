@ok
<?php

require_once 'kphp_tester_include.php';

// only for testing kphp with new libcurl version

function test_new_options() {
  $c = curl_init();
  var_dump(curl_setopt($c, CURLOPT_SSL_OPTIONS, CURLSSLOPT_ALLOW_BEAST | CURLSSLOPT_NO_PARTIALCHAIN));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSL_OPTIONS, CURLSSLOPT_ALLOW_BEAST | CURLSSLOPT_NO_PARTIALCHAIN));
  var_dump(curl_setopt($c, CURLOPT_UPKEEP_INTERVAL_MS, 10000));
  var_dump(curl_setopt($c, CURLOPT_UPLOAD_BUFFERSIZE, 100000));
  var_dump(curl_setopt($c, CURLOPT_SSL_EC_CURVES, "smth"));

  var_dump(curl_setopt($c, CURLOPT_MAIL_RCPT_ALLLOWFAILS, true));
  var_dump(curl_setopt($c, CURLOPT_MAXAGE_CONN, 30));  
  var_dump(curl_setopt($c, CURLOPT_MAXFILESIZE_LARGE, 1 << 48));
  var_dump(curl_setopt($c, CURLOPT_MAXLIFETIME_CONN, 30));   
  var_dump(curl_setopt($c, CURLOPT_SASL_AUTHZID, "Ursel"));        
  curl_close($c);
}

test_new_options();