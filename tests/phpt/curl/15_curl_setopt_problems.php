@ok
<?php

require_once 'kphp_tester_include.php';

// only for testing kphp with new libcurl version and all dependencies

function test_new_options() {
  $c = curl_init();
  curl_setopt($c, CURLOPT_URL, "https://example.com/");

  var_dump(curl_setopt($c, CURLOPT_SSH_COMPRESSION, true));

  var_dump(curl_setopt($c, CURLOPT_HTTP09_ALLOWED, false));
  var_dump(curl_setopt($c, CURLOPT_STREAM_WEIGHT, 20));
  var_dump(curl_setopt($c, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_NONE));
  
  // var_dump(curl_setopt($c, CURLOPT_DNS_INTERFACE, "eth0")); // got false in php
  
  var_dump(curl_setopt($c, CURLOPT_PROXY_TLS13_CIPHERS, "kDHE"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_TLSAUTH_PASSWORD, "smth"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_TLSAUTH_TYPE, "SRP"));
  var_dump(curl_setopt($c, CURLOPT_SSH_HOST_PUBLIC_KEY_SHA256, "smth"));
  var_dump(curl_setopt($c, CURLOPT_TLS13_CIPHERS, "smth"));

  var_dump(curl_exec($c));
  curl_close($c);
}

test_new_options();