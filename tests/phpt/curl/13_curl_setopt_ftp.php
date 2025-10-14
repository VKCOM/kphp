@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function test_ftp_auth_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_DEFAULT));
  var_dump(curl_setopt($c, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_SSL));
  var_dump(curl_setopt($c, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_TLS));

  // bad values
  var_dump(kphp && curl_setopt($c, CURLOPT_FTPSSLAUTH, -10));
  var_dump(kphp && curl_setopt($c, CURLOPT_FTPSSLAUTH, 9999999));

  curl_close($c);
}

function test_ftp_file_method_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_MULTICWD));
  var_dump(curl_setopt($c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_NOCWD));
  var_dump(curl_setopt($c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_SINGLECWD));

  // bad values
  var_dump(kphp && curl_setopt($c, CURLOPT_FTP_FILEMETHOD, -10));
  var_dump(kphp && curl_setopt($c, CURLOPT_FTP_FILEMETHOD, 9999999));

  curl_close($c);
}

test_ftp_auth_option();
test_ftp_file_method_option();
