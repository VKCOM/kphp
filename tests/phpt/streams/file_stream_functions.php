@ok
<?php

function test_file_stream_functions() {
  $filename = dirname(__FILE__) . '/../../../docs/_config.yml';
  $stream = fopen($filename, 'r');
  var_dump(fread($stream, 100));
  var_dump(fpassthru($stream));
  var_dump(feof($stream));
  var_dump(rewind($stream));
  var_dump(fgets($stream, 200));
  var_dump(fseek($stream, ftell($stream)));
  var_dump(fgetc($stream));
  fclose($stream);
  var_dump(file_get_contents($filename));
}

test_file_stream_functions();
