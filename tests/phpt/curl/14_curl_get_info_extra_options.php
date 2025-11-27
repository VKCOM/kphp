@ok k2_skip
<?php

function test_get_info_all_options() {
  $c = curl_init();

  $info = curl_getinfo($c);
#ifndef KPHP
  unset($info["content_type"]);
  unset($info["redirect_url"]);
  unset($info["certinfo"]);
  unset($info["http_version"]);
  unset($info["protocol"]);
  unset($info["ssl_verifyresult"]);
  unset($info["scheme"]);
  unset($info["appconnect_time_us"]);
  unset($info["connect_time_us"]);
  unset($info["namelookup_time_us"]);
  unset($info["pretransfer_time_us"]);
  unset($info["redirect_time_us"]);
  unset($info["starttransfer_time_us"]);
  unset($info["total_time_us"]);
  // Extra options which are unsupported in K2
//   unset($info["ssl_verify_result"]);
//   unset($info["speed_download"]);
//   unset($info["speed_upload"]);
//   unset($info["upload_content_length"]);
  $info["request_header"] = "";
#endif
  $info["filetime"] = 0;

  var_dump($info);

  curl_close($c);
}

function test_get_info_each_option() {
  $c = curl_init("http://example.com");

  var_dump(curl_getinfo($c, CURLINFO_EFFECTIVE_URL));
  var_dump(curl_getinfo($c, CURLINFO_HTTP_CODE));
  var_dump(curl_getinfo($c, CURLINFO_RESPONSE_CODE));
  var_dump(curl_getinfo($c, CURLINFO_TOTAL_TIME));
  var_dump(curl_getinfo($c, CURLINFO_NAMELOOKUP_TIME));
  var_dump(curl_getinfo($c, CURLINFO_CONNECT_TIME));
  var_dump(curl_getinfo($c, CURLINFO_PRETRANSFER_TIME));
  var_dump(curl_getinfo($c, CURLINFO_STARTTRANSFER_TIME));
  var_dump(curl_getinfo($c, CURLINFO_REDIRECT_COUNT));
  var_dump(curl_getinfo($c, CURLINFO_REDIRECT_TIME));
  var_dump(curl_getinfo($c, CURLINFO_SIZE_UPLOAD));
  var_dump(curl_getinfo($c, CURLINFO_SIZE_DOWNLOAD));
  var_dump(curl_getinfo($c, CURLINFO_HEADER_SIZE));
  var_dump(curl_getinfo($c, CURLINFO_REQUEST_SIZE));
  var_dump(curl_getinfo($c, CURLINFO_REQUEST_SIZE));
  var_dump(curl_getinfo($c, CURLINFO_CONTENT_LENGTH_DOWNLOAD));
  var_dump(curl_getinfo($c, CURLINFO_CONTENT_TYPE));
  var_dump(curl_getinfo($c, CURLINFO_REDIRECT_URL));
  var_dump(curl_getinfo($c, CURLINFO_PRIMARY_IP));
  var_dump(curl_getinfo($c, CURLINFO_PRIMARY_PORT));
  var_dump(curl_getinfo($c, CURLINFO_LOCAL_IP));
  var_dump(curl_getinfo($c, CURLINFO_LOCAL_PORT));
  var_dump(curl_getinfo($c, CURLINFO_HTTP_CONNECTCODE));
  var_dump(curl_getinfo($c, CURLINFO_OS_ERRNO));
  var_dump(curl_getinfo($c, CURLINFO_CONDITION_UNMET));
  var_dump(curl_getinfo($c, CURLINFO_NUM_CONNECTS));
  // Extra options which are unsupported in K2
  var_dump(curl_getinfo($c, CURLINFO_SPEED_DOWNLOAD));
  var_dump(curl_getinfo($c, CURLINFO_CONTENT_LENGTH_UPLOAD));
  var_dump(curl_getinfo($c, CURLINFO_SSL_VERIFYRESULT));
  var_dump(curl_getinfo($c, CURLINFO_SPEED_UPLOAD));
  var_dump(curl_getinfo($c, CURLINFO_SPEED_UPLOAD));
  var_dump(curl_getinfo($c, CURLINFO_HTTPAUTH_AVAIL));
  var_dump(curl_getinfo($c, CURLINFO_PROXYAUTH_AVAIL));
  var_dump(curl_getinfo($c, CURLINFO_FTP_ENTRY_PATH));
  var_dump(curl_getinfo($c, CURLINFO_APPCONNECT_TIME));
  var_dump(curl_getinfo($c, CURLINFO_RTSP_CLIENT_CSEQ));
  var_dump(curl_getinfo($c, CURLINFO_RTSP_CSEQ_RECV));
  var_dump(curl_getinfo($c, CURLINFO_RTSP_SERVER_CSEQ));
  var_dump(curl_getinfo($c, CURLINFO_RTSP_SESSION_ID));

  // bad options
  var_dump(curl_getinfo($c, -10));
  var_dump(curl_getinfo($c, 89984894));

  curl_close($c);
}

test_get_info_all_options();
test_get_info_each_option();
