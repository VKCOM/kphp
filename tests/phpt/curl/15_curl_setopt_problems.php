@ok
<?php

require_once 'kphp_tester_include.php';

// only for testing kphp with new libcurl version and all dependencies

function test_new_options() {
  $c = curl_init();
  curl_setopt($c, CURLOPT_URL, "https://example.com/");

  var_dump(curl_setopt($c, CURLOPT_SSH_COMPRESSION, true));              // got 48 error

  var_dump(curl_setopt($c, CURLOPT_HTTP09_ALLOWED, false));
  var_dump(curl_setopt($c, CURLOPT_STREAM_WEIGHT, 20));
  var_dump(curl_setopt($c, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_NONE));  // got 48 error
  
  var_dump(curl_setopt($c, CURLOPT_DNS_INTERFACE, "eth0"));              // got 48 error
  
  var_dump(curl_setopt($c, CURLOPT_PROXY_TLS13_CIPHERS, "kDHE"));        // got 4 error
  var_dump(curl_setopt($c, CURLOPT_PROXY_TLSAUTH_PASSWORD, "smth"));     // got 48 error
  var_dump(curl_setopt($c, CURLOPT_PROXY_TLSAUTH_TYPE, "SRP"));          // got 48 error, needs OpenSSL with TLS-SRP
  var_dump(curl_setopt($c, CURLOPT_SSH_HOST_PUBLIC_KEY_SHA256, "smth")); // got 48 error, needs libssh2
  var_dump(curl_setopt($c, CURLOPT_TLS13_CIPHERS, "smth"));              // got 4 error

  var_dump(curl_exec($c));
  curl_close($c);
}

// test_new_options();