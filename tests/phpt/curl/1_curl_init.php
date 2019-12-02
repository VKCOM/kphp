@ok
<?php

#ifndef KittenPHP
$counter = 0;
#endif

function test_init($url, $need_close) {
  $c = curl_init($url);
  if ($need_close) {
    curl_close($c);
  }
#ifndef KittenPHP
  global $counter;
  var_dump(++$counter);
  return;
#endif
  var_dump($c);
}

test_init("", false);
test_init("http://example.com", false);
test_init("wtf", false);
test_init("", true);
test_init("http://example.com", true);
test_init("wtf", true);
