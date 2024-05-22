@ok
<?php

require_once 'kphp_tester_include.php';

function test_new_options() {
  $c = curl_init();

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

  var_dump(curl_setopt($c, CURLOPT_HTTP09_ALLOWED, false));
  var_dump(curl_setopt($c, CURLOPT_SOCKS5_AUTH, CURLAUTH_GSSAPI));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSL_VERIFYHOST, 2));

  var_dump(curl_setopt($c, CURLOPT_TIMEVALUE, 300));
  var_dump(curl_setopt($c, CURLOPT_TIMEVALUE_LARGE, 1<<12));
  var_dump(curl_setopt($c, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFUNMODSINCE));
  var_dump(curl_setopt($c, CURLOPT_CONNECT_TO, ['example.com: server1.example.com']));
  var_dump(curl_setopt($c, CURLOPT_PROXYHEADER, ["http://proxy.example.com:80"]));
  var_dump(curl_setopt($c, CURLOPT_ENCODING, "gzip"));
  var_dump(curl_setopt($c, CURLOPT_ABSTRACT_UNIX_SOCKET, "smth"));
  var_dump(curl_setopt($c, CURLOPT_DEFAULT_PROTOCOL, "https"));

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
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLKEY, "$filename.pem"));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLKEYTYPE, "PEM"));
  var_dump(curl_setopt($c, CURLOPT_SERVICE_NAME, "custom"));
  var_dump(curl_setopt($c, CURLOPT_UNIX_SOCKET_PATH, "smth"));
  var_dump(curl_setopt($c, CURLOPT_XOAUTH2_BEARER, "smth"));
  curl_close($c);
  exec("rm ./$filename.pem");
  echo "deleting $filename.pem file\n";
}

function test_proxy_ssl_version_option() {
  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_DEFAULT));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_TLSv1));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_TLSv1_0));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_TLSv1_1));
  var_dump(curl_setopt($c, CURLOPT_PROXY_SSLVERSION, CURL_SSLVERSION_TLSv1_2));
  curl_close($c);
}

test_new_options();
test_proxy_ssl_version_option();