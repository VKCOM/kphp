@ok
<?php

require_once 'kphp_tester_include.php';

// this test is not ready yet
// only for testing kphp with new order of stages in compiler.cpp

$length = 0;

function test_stream_options() {
  $c = curl_init("http://localhost:8080");
  $page_info = 'phpinfo();';
  $filename = "simple_page.php";
  $cmd = "nohup php -S localhost:8080 ./$filename > /dev/null 2>&1 & echo $!";
  $fh = fopen("file.txt", "w+");

  $callback = function ($ch, $str) {
      global $length;
      $length += strlen($str);
      if ($length >= 100) return -1;
      return $length;
  };

  // var_dump(curl_setopt($c, CURLOPT_WRITEHEADER, $fh));
  // var_dump(curl_setopt($c, CURLOPT_HEADERFUNCTION, $callback));
  var_dump(curl_setopt($c, CURLOPT_WRITEFUNCTION, $callback));
  var_dump(curl_setopt($c, CURLOPT_FILE, $fh));

  exec("touch $filename && echo \"<?php\" > .$filename");
  exec("echo \"$page_info\" >> .$filename");
  exec($cmd, $op);
  var_dump(curl_exec($c));

  var_dump($fh);
  exec("kill $op[0]");
  fwrite(fopen("./test.txt", "w+"), $fh);
  curl_close($c);
  exec("rm ./$filename");
}

//test_stream_options();