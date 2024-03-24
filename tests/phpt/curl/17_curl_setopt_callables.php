@ok
<?php

require_once 'kphp_tester_include.php';

function test_writefunction_option() {
  $callback = function ($c, $str) {
    var_dump($str);
    return strlen($str);
  };

  $c = curl_init();
  var_dump(curl_setopt($c, CURLOPT_WRITEFUNCTION, $callback));
  curl_close($c);
}

function test_headerfunction_option() {
  $callback = function ($c, $str) {
    var_dump($str);
    return strlen($str);
  };

  $c = curl_init();
  var_dump(curl_setopt($c, CURLOPT_HEADERFUNCTION, $callback));
  curl_close($c);
}

function test_progressfunction_option() {
  $f = fopen("test_progressfunction_option", "w+");
  $progress_callback = function($resource, $download_size, $downloaded, $upload_size, $uploaded) {
    if($download_size > 0) {
      echo round(($downloaded * 100) / $download_size);
      echo "\n";
    }
    usleep(1000*5);
    return 0;
  };

  $c = curl_init();
  var_dump(curl_setopt($c, CURLOPT_FILE, $f));
  var_dump(curl_setopt($c, CURLOPT_PROGRESSFUNCTION, $progress_callback));
  var_dump(curl_setopt($c, CURLOPT_NOPROGRESS, false));

  var_dump(fclose($f));
  curl_close($c);
  exec("rm ./test_progressfunction_option");
}

function test_readfunction_option() {
  global $port, $text;
  $filename_in = "test_file.txt";
  $fh_in = fopen("$filename_in", "w+");

  $callback = function($c, $fh, $length) {
    $result = fread($fh, $length);
    return $result ?: "";
  };

  $c = curl_init();

  var_dump(curl_setopt($c, CURLOPT_PUT, true));
  var_dump(curl_setopt($c, CURLOPT_INFILE, $fh_in));
  var_dump(curl_setopt($c, CURLOPT_READFUNCTION, $callback));

  var_dump(fclose($fh_in));
  curl_close($c);
  exec("rm ./$filename_in");
}

test_writefunction_option();
test_headerfunction_option();
test_progressfunction_option();
test_readfunction_option();