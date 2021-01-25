@ok
<?php

$URLS = [
  '64.246.30.37',
  'http://64.246.30.37',
  'http://64.246.30.37/',
  '64.246.30.37/',
  '64.246.30.37:80/',
  'php.net',
  'php.net/',
  'http://php.net',
  'http://php.net/',
  'www.php.net',
  'www.php.net/',
  'http://www.php.net',
  'http://www.php.net/',
  'www.php.net:80',
  'http://www.php.net:80',
  'http://www.php.net:80/',
  'http://www.php.net/index.php',
  'www.php.net/?',
  'www.php.net:80/?',
  'http://www.php.net/?',
  'http://www.php.net:80/?',
  'http://www.php.net:80/index.php',
  'http://www.php.net:80/foo/bar/index.php',
  'http://www.php.net:80/this/is/a/very/deep/directory/structure/and/file.php',
  'http://www.php.net:80/this/is/a/very/deep/directory/structure/and/file.php?lots=1&of=2&parameters=3&too=4&here=5',
  'http://www.php.net:80/this/is/a/very/deep/directory/structure/and/',
  'http://www.php.net:80/this/is/a/very/deep/directory/structure/and/file.php',
  'http://www.php.net:80/this/../a/../deep/directory',
  'http://www.php.net:80/this/../a/../deep/directory/',
  'http://www.php.net:80/this/is/a/very/deep/directory/../file.php',
  'http://www.php.net:80/index.php',
  'http://www.php.net:80/index.php?',
  'http://www.php.net:80/#foo',
  'http://www.php.net:80/?#',
  'http://www.php.net:80/?test=1',
  'http://www.php.net/?test=1&',
  'http://www.php.net:80/?&',
  'http://www.php.net:80/index.php?test=1&',
  'http://www.php.net/index.php?&',
  'http://www.php.net:80/index.php?foo&',
  'http://www.php.net/index.php?&foo',
  'http://www.php.net:80/index.php?test=1&test2=char&test3=mixesCI',
  'www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123',
  'http://secret@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123',
  'http://secret:@www.php.net/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123',
  'http://:hideout@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123',
  'http://secret:hideout@www.php.net/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123',
  'http://secret:hid:out@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123',
  'nntp://news.php.net',
  'ftp://ftp.gnu.org/gnu/glic/glibc.tar.gz',
  'zlib:http://foo@bar',
  'zlib:filename.txt',
  'zlib:/path/to/my/file/file.txt',
  'foo://foo@bar',
  'mailto:me@mydomain.com',
  '/foo.php?a=b&c=d',
  'foo.php?a=b&c=d',
  'http://user:passwd@www.example.com:8080?bar=1&boom=0',
  'http://user_me-you:my_pas-word@www.example.com:8080?bar=1&boom=0',
  'file:///path/to/file',
  'file://path/to/file',
  'file:/path/to/file',
  'http://1.2.3.4:/abc.asp?a=1&b=2',
  'http://foo.com#bar',
  'scheme:',
  'foo+bar://baz@bang/bla',
  'gg:9130731',
  'http://user:@pass@host/path?argument?value#etc',
  'http://10.10.10.10/:80',
  'http://x:?',
  'x:blah.com',
  'x:/blah.com',
  'x://::abc/?',
  'http://::?',
  'http://::#',
  'x://::6.5',
  'http://?:/',
  'http://@?:/',
  'file:///:',
  'file:///a:/',
  'file:///ab:/',
  'file:///a:/',
  'file:///@:/',
  'file:///:80/',
  '[]',
  'http://[x:80]/',
  '',
  '/',
  '/rest/Users?filter={"id":"123"}',

  // Severely malformed URLs that do not parse:
  'http:///blah.com',
  'http://:80',
  'http://user@:80',
  'http://user:pass@:80',
  'http://:',
  'http://@/',
  'http://@:/',
  'http://:/',
  'http://?',
  'http://#',
  'http://?:',
  'http://:?',
  'http://blah.com:123456',
  'http://blah.com:abcdef',

  // from other tickets
  '//example.org',
  'http://user:pass@host',
  '//user:pass@host',
  '//user@host',
  'http://example.com/path/script.html?t=1#fragment?data',
  'http://example.com/path/script.html#fragment?data',
  ':',
  '//example.org:81/hi?a=b#c=d',
  '//example.org/hi?a=b#c=d',
  'http://example.com:80#@google.com/',
  'http://example.com:80?@google.com/',
  '//php.net/path.php?query=a:b',

  // some excluded, as they produce different results in PHP5 and PHP7, for example:
  // (our KPHP realization should work identically to PHP7 in there cases)
  // '//username@php.net/path?query=1:2',
  // '//php.net/path?query=1:2',
  // '/busca/?fq=B:20001',
];

// Jessie doesn't have fresh PHP
// TODO: Drop
$is_jessie = strpos(file_get_contents("/etc/issue"), "Debian GNU/Linux 8") !== false;
if (!$is_jessie) {
  $URLS[] = 'http://secret@hideout@www.php.net:80/index.php?test=1&test2=char&test3=mixesCI#some_page_ref123';
}


foreach($URLS as $url) {
  echo "--> $url\n";
  $url_arr = parse_url($url);
  ksort($url_arr);
  var_dump($url_arr);
}
echo "\n\n\n";
foreach($URLS as $url) {
  echo "--> $url\n";
  var_dump(parse_url($url, PHP_URL_SCHEME));
  var_dump(parse_url($url, PHP_URL_FRAGMENT));
  var_dump(parse_url($url, PHP_URL_HOST));
}
