@ok php8
<?php

function dump_and_return($string) {
  var_dump($string);
  return $string;
}

var_dump(match ('foo') {
  'foo' => dump_and_return('foo'),
  'bar' => dump_and_return('bar'),
});
