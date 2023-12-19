@ok
<?php

require_once 'kphp_tester_include.php';

function test_new_options() {
  $c = curl_init();
  curl_setopt($c, CURLOPT_URL, "https://example.com/");

  var_dump(curl_setopt($c, CURLOPT_SSL_VERIFYSTATUS, 1));
  var_dump(curl_setopt($c, CURLOPT_DISALLOW_USERNAME_IN_URL, 1));
  var_dump(curl_setopt($c, CURLOPT_CERTINFO, 1));
  var_dump(curl_setopt($c, CURLOPT_NOPROGRESS, false));
  var_dump(curl_setopt($c, CURLOPT_NOSIGNAL, false));
  var_dump(curl_setopt($c, CURLOPT_PATH_AS_IS, true));
  var_dump(curl_setopt($c, CURLOPT_PIPEWAIT, 1));
  var_dump(curl_setopt($c, CURLOPT_SASL_IR, 1));
  var_dump(curl_setopt($c, CURLOPT_EXPECT_100_TIMEOUT_MS, 200));
  var_dump(curl_setopt($c, CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS, 1000));
  var_dump(curl_setopt($c, CURLOPT_HEADEROPT, CURLHEADER_SEPARATE));
  var_dump(curl_setopt($c, CURLOPT_POSTREDIR, 1));
  var_dump(curl_setopt($c, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS|CURLPROTO_SCP));
  var_dump(curl_setopt($c, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_FILE));
  var_dump(curl_setopt($c, CURLOPT_DNS_SHUFFLE_ADDRESSES, true));
  var_dump(curl_setopt($c, CURLOPT_HAPROXYPROTOCOL, false));
  var_dump(curl_setopt($c, CURLOPT_DNS_USE_GLOBAL_CACHE, true));
  var_dump(curl_setopt($c, CURLOPT_FTPAPPEND, true));
  var_dump(curl_setopt($c, CURLOPT_FTPLISTONLY, true));
  var_dump(curl_setopt($c, CURLOPT_KEEP_SENDING_ON_ERROR, false));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSL_VERIFYPEER, false));
  var_dump(curl_setopt($c, CURLOPT_SUPPRESS_CONNECT_HEADERS, true));
  var_dump(curl_setopt($c, CURLOPT_TCP_FASTOPEN, true));
  var_dump(curl_setopt($c, CURLOPT_TFTP_NO_OPTIONS, true));
  // var_dump(curl_setopt($c, CURLOPT_SSH_COMPRESSION, true));       // got 48 error
  // var_dump(curl_setopt($c, CURLOPT_SSL_FALSESTART, false));       // got 4 error

  var_dump(curl_setopt($c, CURLOPT_HTTP09_ALLOWED, false));
  //var_dump(@curl_setopt($c, CURLOPT_MAIL_RCPT_ALLLOWFAILS, true)); // got warning and NULL from php
  //var_dump(@curl_setopt($c, CURLOPT_MAXAGE_CONN, 30));             // got warning and NULL from php
  //var_dump(curl_setopt($c, CURLOPT_MAXFILESIZE_LARGE, 1<<58));     // got NULL from php
  //var_dump(curl_setopt($c, CURLOPT_MAXLIFETIME_CONN, 30));         // got NULL from php
  var_dump(curl_setopt($c, CURLOPT_SOCKS5_AUTH, CURLAUTH_GSSAPI));
  var_dump(curl_setopt($c, CURLOPT_SSL_OPTIONS, CURLSSLOPT_ALLOW_BEAST | CURLSSLOPT_NO_PARTIALCHAIN));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSL_OPTIONS, CURLSSLOPT_ALLOW_BEAST | CURLSSLOPT_NO_PARTIALCHAIN));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSL_VERIFYHOST, 2));
  
  test_proxy_ssl_version_option(); // testing CURLOPT_PROXY_SSLVERSION option

  //var_dump(curl_setopt($c, CURLOPT_STREAM_WEIGHT, 20)); // dont work (very bad)

  var_dump(curl_setopt($c, CURLOPT_TIMEVALUE, 300));
  var_dump(curl_setopt($c, CURLOPT_TIMEVALUE_LARGE, 1<<12));
  var_dump(curl_setopt($c, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFUNMODSINCE));
  var_dump(curl_setopt($c, CURLOPT_UPKEEP_INTERVAL_MS, 10000));
  var_dump(curl_setopt($c, CURLOPT_UPLOAD_BUFFERSIZE, 100000));
  var_dump(curl_setopt($c, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_NONE)); // got 48 error
  var_dump(curl_setopt($c, CURLOPT_CONNECT_TO, ['example.com: server1.example.com']));
  var_dump(curl_setopt($c, CURLOPT_PROXYHEADER, ["http://proxy.example.com:80"]));
  var_dump(curl_setopt($c, CURLOPT_ENCODING, "gzip"));
  var_dump(curl_setopt($c, CURLOPT_ABSTRACT_UNIX_SOCKET, "smth"));
  var_dump(curl_setopt($c, CURLOPT_DEFAULT_PROTOCOL, "https"));

  // var_dump(curl_setopt($c, CURLOPT_DNS_INTERFACE, "eth0")); // got 48 error
  var_dump(curl_setopt($c, CURLOPT_KEYPASSWD, "smth"));
  var_dump(curl_setopt($c, CURLOPT_LOGIN_OPTIONS, "AUTH=*"));
  var_dump(curl_setopt($c, CURLOPT_PINNEDPUBLICKEY, "sha256//smth"));
  var_dump(curl_setopt($c, CURLOPT_PRE_PROXY, "smth"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SERVICE_NAME, "smth"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_CAINFO, "smth"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_CAPATH, "smth"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_KEYPASSWD, "smth"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_PINNEDPUBLICKEY, "sha256//smth"));
  
  echo "creating pem file\n...";
  $filename = "test";
  exec("touch ./$filename.pem && echo 'smth' > $filename.pem");
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLCERT, "$filename.pem"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLCERTTYPE, "$filename.pem"));

  var_dump(curl_setopt($c, CURLOPT_PROXY_SSL_CIPHER_LIST, "TLSv1"));
  // var_dump(curl_setopt($c, CURLOPT_PROXY_TLS13_CIPHERS, "kDHE")); // got 4 error
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLKEY, "$filename.pem"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLKEYTYPE, "PEM"));
  // var_dump(curl_setopt($c, CURLOPT_PROXY_TLSAUTH_PASSWORD, "smth")); // got 48 error
  // var_dump(curl_setopt($c, CURLOPT_PROXY_TLSAUTH_TYPE, "SRP")); // got 48 error, needs OpenSSL with TLS-SRP
  // var_dump(curl_setopt($c, CURLOPT_SASL_AUTHZID, "Ursel")); // got NULL from PHP
  var_dump(curl_setopt($c, CURLOPT_SERVICE_NAME, "custom"));
  // var_dump(curl_setopt($c, CURLOPT_SSH_HOST_PUBLIC_KEY_SHA256, "smth")); // got 48 error, needs libssh2
  var_dump(curl_setopt($c, CURLOPT_SSL_EC_CURVES, "smth"));
  // var_dump(curl_setopt($c, CURLOPT_TLS13_CIPHERS, "smth")); // got 4 error (doesn't supported)
  var_dump(curl_setopt($c, CURLOPT_UNIX_SOCKET_PATH, "smth"));
  var_dump(curl_setopt($c, CURLOPT_XOAUTH2_BEARER, "smth"));

  var_dump(curl_exec($c) == true);
  curl_close($c);
  //exec("rm ./$filename.pem");
  //echo "deleting $filename.pem file\n";
}

function test_stream_options() {
  $c = curl_init("http://localhost:8080");
  $page_info = 'phpinfo()';
  $filename = "kphp_simple_page.php";
  #ifndef KPHP
  $filename = "php_simple_page.php";
  #endif
  $cmd = "nohup php -S localhost:8080 ./$filename > /dev/null 2>&1 & echo $!";
  $fh = fopen("file.txt", "w+");
  $length = 0;
  $callback = function ($ch, $str) use ($length) {
      $length += strlen($str);
      if ($length >= 100) return -1;
      return $length;
  };

  // var_dump(curl_setopt($c, CURLOPT_WRITEHEADER, $fh));
  // var_dump(curl_setopt($c, CURLOPT_HEADERFUNCTION, $callback));
  var_dump(curl_setopt($c, CURLOPT_WRITEFUNCTION, $callback));
  var_dump(curl_setopt($c, CURLOPT_FILE, $fh));

  exec("touch $filename && echo \"<?php\n echo '$page_info';\" > .$filename");
  exec($cmd, $op);
  var_dump(curl_exec($c));

  #ifndef KPHP
  echo "hello from PHP\n";
  #endif

  // var_dump($fh);
  // exec("kill $op[0]");
  // fwrite(fopen("./test.txt", "w+"), $fh);
  // curl_close($c);
  // exec("rm ./simple_page.php");
}

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
  //var_dump(curl_setopt($c, CURLOPT_MAIL_RCPT, "bad value"));
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
  // var_dump(kphp && curl_setopt($c, CURLOPT_PROXYTYPE, -10));
  // var_dump(kphp && curl_setopt($c, CURLOPT_PROXYTYPE, 9999999));

  curl_close($c);
}

function test_proxy_ssl_version_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_DEFAULT));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_TLSv1));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_TLSv1_0));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_TLSv1_1));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_TLSv1_2));
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
  // var_dump(kphp && curl_setopt($c, CURLOPT_SSLVERSION, -10));
  // var_dump(kphp && curl_setopt($c, CURLOPT_SSLVERSION, 9999999));

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
  // var_dump(kphp && curl_setopt($c, CURLOPT_IPRESOLVE, -10));
  // var_dump(kphp && curl_setopt($c, CURLOPT_IPRESOLVE, 9999999));

  curl_close($c);
}

function test_ftp_auth_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_DEFAULT));
  var_dump(curl_setopt($c, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_SSL));
  var_dump(curl_setopt($c, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_TLS));

  // bad values
  // var_dump(kphp && curl_setopt($c, CURLOPT_FTPSSLAUTH, -10));
  // var_dump(kphp && curl_setopt($c, CURLOPT_FTPSSLAUTH, 9999999));

  curl_close($c);
}

function test_ftp_file_method_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_MULTICWD));
  var_dump(curl_setopt($c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_NOCWD));
  var_dump(curl_setopt($c, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_SINGLECWD));

  // bad values
  // var_dump(kphp && curl_setopt($c, CURLOPT_FTP_FILEMETHOD, -10));
  // var_dump(kphp && curl_setopt($c, CURLOPT_FTP_FILEMETHOD, 9999999));

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


function test_write_function_option() {
  $fhandle = "hello world\n";
  $ch = curl_init();
  $callback = function ($ch, $str) use ($fhandle) {
      $rval = fwrite($fhandle, $str);
      return $rval ?: 0;
  };

  var_dump(curl_setopt($ch, CURLOPT_WRITEFUNCTION, $callback));
  curl_close($ch);
}

function test_write_header_function_option() {
  $fhandle = "hello world\n";
  $ch = curl_init();
  $callback = function ($ch, $str) use ($fhandle) {
      $rval = fwrite($fhandle, $str);
      return $rval ?: 0;
  };

  var_dump(curl_setopt($ch, CURLOPT_WRITEHEADER, $fhandle));
  var_dump(curl_setopt($ch, CURLOPT_HEADERFUNCTION, $callback));
  curl_close($ch);
}

function test_progress_function_option() {
  $ch = curl_init();

  $callback = function ($resource,$download_size, $downloaded, $upload_size, $uploaded)
  {
    if($download_size > 0)
        echo $downloaded / $download_size  * 100;
    return 0;
  };
  
  var_dump(curl_setopt($ch, CURLOPT_NOPROGRESS, false));
  var_dump(curl_setopt($ch, CURLOPT_PROGRESSFUNCTION, $callback));
  curl_close($ch);
}

function test_read_function_option() {
  $ch = curl_init();

  $stream = fopen("php://stdin", "r"); // wont work in kphp

  $callback = function ($ch, $stream, $length) {
      $data = fread($stream, $length);
      var_dump($data);
      return ($data !== false) ? $data : "";
  };

  var_dump(curl_setopt($ch, CURLOPT_READFUNCTION, $callback));
  var_dump(curl_setopt($ch, CURLOPT_INFILE, $stream));
  curl_close($ch);
}

test_new_options();
// test_stream_options();

test_long_options();
test_string_options();
test_linked_list_options();

test_proxy_type_option();
// test_proxy_ssl_version_option();
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

// test_write_function_option();
// test_write_header_function_option();
// test_progress_function_option();
// test_read_function_option();