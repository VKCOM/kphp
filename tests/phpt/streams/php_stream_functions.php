@ok
<?php

function test_php_stream_functions() {
  $stream = fopen('php://stdout', 'w');
  $data = file_get_contents(__DIR__ . '/csv_input.txt');
  var_dump(fwrite($stream, $data));
}

test_php_stream_functions();
