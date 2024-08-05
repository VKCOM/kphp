@ok k2_skip
<?php

#ifndef KPHP
$counter = 0;
#endif

/**
 * @param string $url
 * @param bool $need_close
 */
function test_init($url, $need_close) {
  $c = curl_init($url);
  if ($need_close) {
    curl_close($c);
  }
#ifndef KPHP
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
