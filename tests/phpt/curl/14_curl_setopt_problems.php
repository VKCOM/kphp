@ok
<?php

require_once 'kphp_tester_include.php';

function test_new_options() {
  $c = curl_init();
  curl_setopt($c, CURLOPT_URL, "https://example.com/");

  var_dump(curl_setopt($c, CURLOPT_SSH_COMPRESSION, true));       // got 48 error
  var_dump(curl_setopt($c, CURLOPT_SSL_FALSESTART, false));       // got 4 error

  var_dump(curl_setopt($c, CURLOPT_HTTP09_ALLOWED, false));
  var_dump(@curl_setopt($c, CURLOPT_MAIL_RCPT_ALLLOWFAILS, true)); // got warning and NULL from php
  var_dump(@curl_setopt($c, CURLOPT_MAXAGE_CONN, 30));             // got warning and NULL from php
  var_dump(curl_setopt($c, CURLOPT_MAXFILESIZE_LARGE, 1<<58));     // got NULL from php
  var_dump(curl_setopt($c, CURLOPT_MAXLIFETIME_CONN, 30));         // got NULL from php

  var_dump(curl_setopt($c, CURLOPT_STREAM_WEIGHT, 20)); // dont work (very bad)

  var_dump(curl_setopt($c, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_NONE)); // got 48 error
  

  var_dump(curl_setopt($c, CURLOPT_DNS_INTERFACE, "eth0")); // got 48 error
  
  var_dump(curl_setopt($c, CURLOPT_PROXY_TLS13_CIPHERS, "kDHE")); // got 4 error
  var_dump(curl_setopt($c, CURLOPT_PROXY_TLSAUTH_PASSWORD, "smth")); // got 48 error
  var_dump(curl_setopt($c, CURLOPT_PROXY_TLSAUTH_TYPE, "SRP")); // got 48 error, needs OpenSSL with TLS-SRP
  var_dump(curl_setopt($c, CURLOPT_SASL_AUTHZID, "Ursel")); // got NULL from PHP
  var_dump(curl_setopt($c, CURLOPT_SSH_HOST_PUBLIC_KEY_SHA256, "smth")); // got 48 error, needs libssh2
  var_dump(curl_setopt($c, CURLOPT_TLS13_CIPHERS, "smth")); // got 4 error (doesn't supported)

  var_dump(curl_exec($c) == true);
  curl_close($c);
}