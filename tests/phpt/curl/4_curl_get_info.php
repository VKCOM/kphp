@ok
<?php

function test_get_info_all() {
  $c = curl_init();

  $info = curl_getinfo($c);
#ifndef KittenPHP
  unset($info["content_type"]);
  unset($info["redirect_url"]);
  unset($info["certinfo"]);
  $info["request_header"] = "";
#endif
  $info["filetime"] = 0;

  var_dump($info);

  curl_close($c);
}

function test_get_info_header() {
  $c = curl_init();

#ifndef KittenPHP
  var_dump("");
  if (0)
#endif
  var_dump(curl_getinfo($c, CURLINFO_HEADER_OUT));

  curl_close($c);
}

function test_get_info_options() {
  $c = curl_init("http://example.com");

  var_dump(curl_getinfo($c, CURLINFO_EFFECTIVE_URL));
  var_dump(curl_getinfo($c, CURLINFO_HTTP_CODE));
  var_dump(curl_getinfo($c, CURLINFO_TOTAL_TIME));
  var_dump(curl_getinfo($c, CURLINFO_NAMELOOKUP_TIME));
  var_dump(curl_getinfo($c, CURLINFO_CONNECT_TIME));
  var_dump(curl_getinfo($c, CURLINFO_PRETRANSFER_TIME));
  var_dump(curl_getinfo($c, CURLINFO_STARTTRANSFER_TIME));
  var_dump(curl_getinfo($c, CURLINFO_REDIRECT_COUNT));
  var_dump(curl_getinfo($c, CURLINFO_REDIRECT_TIME));
  var_dump(curl_getinfo($c, CURLINFO_SIZE_UPLOAD));
  var_dump(curl_getinfo($c, CURLINFO_SIZE_DOWNLOAD));
  var_dump(curl_getinfo($c, CURLINFO_SPEED_UPLOAD));
  var_dump(curl_getinfo($c, CURLINFO_HEADER_SIZE));
  var_dump(curl_getinfo($c, CURLINFO_REQUEST_SIZE));
  var_dump(curl_getinfo($c, CURLINFO_SSL_VERIFYRESULT));
  var_dump(curl_getinfo($c, CURLINFO_REQUEST_SIZE));
  var_dump(curl_getinfo($c, CURLINFO_SSL_VERIFYRESULT));
  var_dump(curl_getinfo($c, CURLINFO_CONTENT_LENGTH_DOWNLOAD));
  var_dump(curl_getinfo($c, CURLINFO_CONTENT_LENGTH_UPLOAD));
  var_dump(curl_getinfo($c, CURLINFO_CONTENT_TYPE));

  // bad options
  var_dump(curl_getinfo($c, -10));
  var_dump(curl_getinfo($c, 89984894));

  curl_close($c);
}

function test_get_private_data() {
  $c = curl_init();

  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));

  var_dump(curl_setopt($c, CURLOPT_PRIVATE, "Hello!"));
  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));

  var_dump(curl_setopt($c, CURLOPT_PRIVATE, null));
  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));

  var_dump(curl_setopt($c, CURLOPT_PRIVATE, 231));
  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));

  var_dump(curl_setopt($c, CURLOPT_PRIVATE, ["hello", 2, "world"]));
  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));

  var_dump(curl_setopt($c, CURLOPT_PRIVATE, false));
  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));

  var_dump(curl_setopt($c, CURLOPT_PRIVATE, true));
  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));

  var_dump(curl_setopt($c, CURLOPT_PRIVATE, 0.21));
  var_dump(curl_getinfo($c, CURLINFO_PRIVATE));

  curl_close($c);
}

test_get_info_all();
test_get_info_header();
test_get_info_options();
test_get_private_data();
