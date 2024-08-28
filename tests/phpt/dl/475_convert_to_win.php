@ok benchmark k2_skip
<?php
$str = "мама мыла раму!!! asfmklasdfasdgbsfbvdf buydfbdvbdfb vdfv dfv";

for ($i = 0; $i < 1000000; $i++) {
  $str_utf = vk_win_to_utf8($str);
}

var_dump ($str);
var_dump ($str_utf);

$text = $str_utf;

echo 'Original : ', $text, PHP_EOL;
echo 'TRANSLIT : ', iconv("UTF-8", "cp1251//TRANSLIT", $text), PHP_EOL;
echo 'IGNORE   : ', iconv("UTF-8", "cp1251//IGNORE", $text), PHP_EOL;
echo 'Plain    : ', iconv("UTF-8", "cp1251", $text), PHP_EOL;

var_dump (setlocale (LC_ALL, "en_US.utf8"));

$text = "This is the Euro symbol 'И'.";

echo 'Original : ', $text, PHP_EOL;
#ifndef KPHP
echo 'TRANSLIT : This is the Euro symbol \'', PHP_EOL;
if (false)
#endif
echo 'TRANSLIT : ', iconv("UTF-8", "ISO-8859-1//TRANSLIT", $text), PHP_EOL;
echo 'IGNORE   : ', iconv("UTF-8", "ISO-8859-1//IGNORE", $text), PHP_EOL;
#ifndef KPHP
echo 'Plain    : This is the Euro symbol \'', PHP_EOL;
if (false)
#endif
echo 'Plain    : ', iconv("UTF-8", "ISO-8859-1", $text), PHP_EOL;
