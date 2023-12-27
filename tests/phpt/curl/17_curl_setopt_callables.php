@ok
<?php

require_once 'kphp_tester_include.php';

// this test is not ready yet
// only for testing kphp with new order of stages in compiler.cpp

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