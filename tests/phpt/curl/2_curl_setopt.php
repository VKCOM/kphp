@ok
<?php

require_once 'kphp_tester_include.php';

function test_long_options() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_CONNECTTIMEOUT, 10));
  var_dump(curl_setopt($c, CURLOPT_TIMEOUT, 125.47));
  var_dump(curl_setopt($c, CURLOPT_PORT, "9999"));
  var_dump(curl_setopt($c, CURLOPT_INFILESIZE, "99 kek"));
  var_dump(curl_setopt($c, CURLOPT_HTTP_TRANSFER_DECODING, false));
  var_dump(curl_setopt($c, CURLOPT_FTP_USE_PRET, true));
  var_dump(curl_setopt($c, CURLOPT_DNS_CACHE_TIMEOUT, null));
  var_dump(curl_setopt($c, CURLOPT_PUT, 897));

  var_dump(curl_setopt($c, CURLOPT_SSL_ENABLE_ALPN, 1));
  var_dump(curl_setopt($c, CURLOPT_SSL_ENABLE_NPN, 1));
  var_dump(curl_setopt($c, CURLOPT_TCP_KEEPALIVE, 1));
  var_dump(curl_setopt($c, CURLOPT_TCP_KEEPIDLE, 5));
  var_dump(curl_setopt($c, CURLOPT_TCP_KEEPINTVL, 12));

  curl_close($c);
}

function test_string_options() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_URL, "who care"));
  var_dump(curl_setopt($c, CURLOPT_PASSWORD, 123456));
  var_dump(curl_setopt($c, CURLOPT_USERNAME, 123.412));
  var_dump(curl_setopt($c, CURLOPT_FTP_ACCOUNT, null));
  var_dump(curl_setopt($c, CURLOPT_PROXY, true));
  var_dump(curl_setopt($c, CURLOPT_RANGE, false));

  curl_close($c);
}

function test_linked_list_options() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_HTTP200ALIASES, ["who", "care"]));
  var_dump(curl_setopt($c, CURLOPT_HTTPHEADER, [123, 456, null]));

  var_dump(curl_setopt($c, CURLOPT_CONNECT_TO, [123, 456, "who care", null]));
  var_dump(curl_setopt($c, CURLOPT_PROXYHEADER, [123, 456, null]));

  var_dump(curl_setopt($c, CURLOPT_POSTQUOTE, ["sdad", 123.412]));
  var_dump(curl_setopt($c, CURLOPT_PREQUOTE, ["foo", "bar", "baz"]));
  var_dump(curl_setopt($c, CURLOPT_QUOTE, [true, false]));
  var_dump(curl_setopt($c, CURLOPT_MAIL_RCPT, []));
  var_dump(curl_setopt($c, CURLOPT_RESOLVE, ["www.example.com:8081:127.0.0.1"]));

  // bad values
  var_dump(curl_setopt($c, CURLOPT_MAIL_RCPT, "bad value"));
  var_dump(curl_setopt($c, CURLOPT_QUOTE, 1));
  var_dump(curl_setopt($c, CURLOPT_POSTQUOTE, null));

  curl_close($c);
}

function test_proxy_type_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_PROXYTYPE, CURLPROXY_HTTP));
  var_dump(curl_setopt($c, CURLOPT_PROXYTYPE, CURLPROXY_HTTP_1_0));
  var_dump(curl_setopt($c, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4));
  var_dump(curl_setopt($c, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4A));
  var_dump(curl_setopt($c, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5));
  var_dump(curl_setopt($c, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME));

  // bad values
  var_dump(kphp && curl_setopt($c, CURLOPT_PROXYTYPE, -10));
  var_dump(kphp && curl_setopt($c, CURLOPT_PROXYTYPE, 9999999));

  curl_close($c);
}

function test_ssl_version_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_SSLVERSION, CURL_SSLVERSION_DEFAULT));
  var_dump(curl_setopt($c, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1));
  var_dump(curl_setopt($c, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv2));
  var_dump(curl_setopt($c, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3));
  var_dump(curl_setopt($c, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_0));
  var_dump(curl_setopt($c, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_1));
  var_dump(curl_setopt($c, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2));

  // bad values
  var_dump(kphp && curl_setopt($c, CURLOPT_SSLVERSION, -10));
  var_dump(kphp && curl_setopt($c, CURLOPT_SSLVERSION, 9999999));

  curl_close($c);
}

function test_auth_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_HTTPAUTH, CURLAUTH_BASIC));
  var_dump(curl_setopt($c, CURLOPT_PROXYAUTH, CURLAUTH_DIGEST));
  // curl-kphp-vk не поддеживает эту опцию
  // var_dump(curl_setopt($c, CURLOPT_HTTPAUTH, CURLAUTH_GSSNEGOTIATE));
  var_dump(curl_setopt($c, CURLOPT_PROXYAUTH, CURLAUTH_NTLM));
  var_dump(curl_setopt($c, CURLOPT_HTTPAUTH, CURLAUTH_ANYSAFE));
  var_dump(curl_setopt($c, CURLOPT_PROXYAUTH, CURLAUTH_ANY));
  var_dump(curl_setopt($c, CURLOPT_HTTPAUTH, CURLAUTH_BASIC|CURLAUTH_NTLM));

  curl_close($c);
}

function test_ip_resolve_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_WHATEVER));
  var_dump(curl_setopt($c, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4));
  var_dump(curl_setopt($c, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6));

  // bad values
  var_dump(kphp && curl_setopt($c, CURLOPT_IPRESOLVE, -10));
  var_dump(kphp && curl_setopt($c, CURLOPT_IPRESOLVE, 9999999));

  curl_close($c);
}

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

function test_post_fields_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_POSTFIELDS, []));
  var_dump(curl_setopt($c, CURLOPT_POSTFIELDS, ["key" => "hello"]));
  var_dump(curl_setopt($c, CURLOPT_POSTFIELDS, "world"));

  curl_close($c);
}

function test_max_recv_speed() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_MAX_RECV_SPEED_LARGE, 98787));
  var_dump(curl_setopt($c, CURLOPT_MAX_SEND_SPEED_LARGE, "123124"));

  curl_close($c);
}

function test_special_options() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_RETURNTRANSFER, 1));
  var_dump(curl_setopt($c, CURLINFO_HEADER_OUT, 1));

  curl_close($c);
}

function test_setopt_array() {
  $c = curl_init();

  var_dump(curl_setopt_array($c, [
    CURLOPT_RETURNTRANSFER => 1,
    CURLINFO_HEADER_OUT => 0,
    CURLOPT_PORT => 99
  ]));

  var_dump(curl_setopt_array($c, [
    CURLOPT_URL => "http",
    CURLOPT_PASSWORD => 1234,
    CURLOPT_PORT => 99,
    CURLOPT_FTP_FILEMETHOD => CURLFTPMETHOD_MULTICWD
  ]));

  curl_close($c);
}

function test_bad_option() {
  $c = curl_init();

  var_dump(kphp && curl_setopt($c, -123, 1));

  curl_close($c);
}


test_long_options();
test_string_options();
test_linked_list_options();

test_proxy_type_option();
test_ssl_version_option();
test_auth_option();
test_ip_resolve_option();
test_ftp_auth_option();
test_ftp_file_method_option();
test_post_fields_option();
test_max_recv_speed();
test_special_options();

test_setopt_array();

test_bad_option();
