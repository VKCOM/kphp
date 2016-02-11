@wa benchmark
<?php
#ifndef KittenPHP
function charToWin($x) {
  if ($x < 0x80) return chr($x);

  static $utf8_to_win_convert = array(
    0x402  => 0x80,
    0x403  => 0x81,
    0x201a => 0x82,
    0x453  => 0x83,
    0x201e => 0x84,
    0x2026 => 0x85,
    0x2020 => 0x86,
    0x2021 => 0x87,
    0x20ac => 0x88,
    0x2030 => 0x89,
    0x409  => 0x8a,
    0x2039 => 0x8b,
    0x40a  => 0x8c,
    0x40c  => 0x8d,
    0x40b  => 0x8e,
    0x40f  => 0x8f,
    0x452  => 0x90,
    0x2018 => 0x91,
    0x2019 => 0x92,
    0x201c => 0x93,
    0x201d => 0x94,
    0x2022 => 0x95,
    0x2013 => 0x96,
    0x2014 => 0x97,
    0xfffd => 0x20, // Anton, 16.05.2009 - было   0x98
    0x2122 => 0x99,
    0x459  => 0x9a,
    0x203a => 0x9b,
    0x45a  => 0x9c,
    0x45c  => 0x9d,
    0x45b  => 0x9e,
    0x45f  => 0x9f,
    0xa0   => 0xa0,
    0x40e  => 0xa1,
    0x45e  => 0xa2,
    0x408  => 0xa3,
    0xa4   => 0xa4,
    0x490  => 0xa5,
    0xa6   => 0xa6,
    0xa7   => 0xa7,
    0x401  => 0xa8,
    0xa9   => 0xa9,
    0x404  => 0xaa,
    0xab   => 0xab,
    0xac   => 0xac,
    0xad   => 0xad,
    0xae   => 0xae,
    0x407  => 0xaf,
    0xb0   => 0xb0,
    0xb1   => 0xb1,
    0x406  => 0xb2,
    0x456  => 0xb3,
    0x491  => 0xb4,
    0xb5   => 0xb5,
    0xb6   => 0xb6,
    0xb7   => 0xb7,
    0x451  => 0xb8,
    0x2116 => 0xb9,
    0x454  => 0xba,
    0xbb   => 0xbb,
    0x458  => 0xbc,
    0x405  => 0xbd,
    0x455  => 0xbe,
    0x457  => 0xbf,
    0x410  => 0xc0,
    0x411  => 0xc1,
    0x412  => 0xc2,
    0x413  => 0xc3,
    0x414  => 0xc4,
    0x415  => 0xc5,
    0x416  => 0xc6,
    0x417  => 0xc7,
    0x418  => 0xc8,
    0x419  => 0xc9,
    0x41a  => 0xca,
    0x41b  => 0xcb,
    0x41c  => 0xcc,
    0x41d  => 0xcd,
    0x41e  => 0xce,
    0x41f  => 0xcf,
    0x420  => 0xd0,
    0x421  => 0xd1,
    0x422  => 0xd2,
    0x423  => 0xd3,
    0x424  => 0xd4,
    0x425  => 0xd5,
    0x426  => 0xd6,
    0x427  => 0xd7,
    0x428  => 0xd8,
    0x429  => 0xd9,
    0x42a  => 0xda,
    0x42b  => 0xdb,
    0x42c  => 0xdc,
    0x42d  => 0xdd,
    0x42e  => 0xde,
    0x42f  => 0xdf,
    0x430  => 0xe0,
    0x431  => 0xe1,
    0x432  => 0xe2,
    0x433  => 0xe3,
    0x434  => 0xe4,
    0x435  => 0xe5,
    0x436  => 0xe6,
    0x437  => 0xe7,
    0x438  => 0xe8,
    0x439  => 0xe9,
    0x43a  => 0xea,
    0x43b  => 0xeb,
    0x43c  => 0xec,
    0x43d  => 0xed,
    0x43e  => 0xee,
    0x43f  => 0xef,
    0x440  => 0xf0,
    0x441  => 0xf1,
    0x442  => 0xf2,
    0x443  => 0xf3,
    0x444  => 0xf4,
    0x445  => 0xf5,
    0x446  => 0xf6,
    0x447  => 0xf7,
    0x448  => 0xf8,
    0x449  => 0xf9,
    0x44a  => 0xfa,
    0x44b  => 0xfb,
    0x44c  => 0xfc,
    0x44d  => 0xfd,
    0x44e  => 0xfe,
    0x44f  => 0xff
  );
  if (isset($utf8_to_win_convert[$x])) return chr($utf8_to_win_convert[$x]);
  if ($x == 8238 || $x == 8236 || $x == 8235) return chr(0xda); // ласипан
  return "&#{$x};";
}

function vk_utf8_to_win($str, $maxlen = 0, $exit_on_error = false) {
  $str .= '';
  if ($maxlen) {
    $str = substr($str, 0, $maxlen * 3);
  }

  $len = strlen($str);
  $out = '';
  $st = 0;
  $acc = 0;
  for ($i = 0; $i < $len; $i++) {
    $c = $str[$i];
    $c0 = ord($c);
    if ($c0 < 0x80) {
      if ($st) {
        if ($exit_on_error) return $str;
        $out .= '?1?';
      }
      $out .= $c;
      $st = 0;
      continue;
    }
    if (($c0 & 0xc0) == 0x80) {
      if (!$st) {
        if ($exit_on_error) return $str;
        $out .= '?2?';
        continue;
      }
      $acc <<= 6;
      $acc += $c0 - 0x80;
      if (!--$st) {
        if ($exit_on_error && $acc < 0x80) return $str;
        $out .= ($acc < 0x80) ? '?3?' : charToWin($acc);
      }
      continue;
    }
    if (($c0 & 0xc0) == 0xc0) {
      if ($st) {
        if ($exit_on_error) return $str;
        $out .= '?4?';
      }
      $c0 -= 0xc0;
      $st = 0;
      if ($c0 < 32) {
        $acc = $c0;
        $st = 1;
      } else if ($c0 < 48) {
        $acc = $c0 - 32;
        $st = 2;
      } else if ($c0 < 56) {
        $acc = $c0 - 48;
        $st = 3;
      } else {
        if ($exit_on_error) return $str;
        $out .= '?5?';
      }
    }
  }
  if ($st) {
    if ($exit_on_error) return $str;
    $out .= '?6?';
  }
  if ($maxlen) {
    $out = substr($out, 0,  $maxlen);
  }
  return $out;
}

function charToUtf($code) {
  $code = intval($code);
  if ($code < 32 || $code == 33 || $code == 34 || $code == 36 || $code == 39 || $code == 60 || $code == 62 || $code == 92 || $code == 8232 || $code == 8233) {
    return "&#{$code};";
  }
  if ($code < 128) {
    return chr($code);
  }
  if ($code < 0x800) {
    return chr(0xc0+($code>>6)).chr(0x80+($code&63));
  }
  if ($code < 0x10000) {
    return chr(0xe0+($code>>12)).chr(0x80+(($code>>6)&63)).chr(0x80+($code&63));
  }
  //if ($code >= 0x1f300 && $code <= 0x1f64f) { // emoji support in API
  if ($code >= 0x1f000 && $code <= 0x1f6c0) { // emoji support in API
    return chr(0xf0+($code>>18)).chr(0x80+(($code>>12)&63)).chr(0x80+(($code>>6)&63)).chr(0x80+($code&63));
  }
  return "$#{$code};";
}

function vk_win_to_utf8($str) {
  $str = iconv("WINDOWS-1251", "UTF-8", $str.'');
  $str = preg_replace("/&#([0-9]+);/e", 'charToUtf("\\1")', $str . '');
  return $str;
}
#endif

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
echo 'TRANSLIT : ', iconv("UTF-8", "ISO-8859-1//TRANSLIT", $text), PHP_EOL;
echo 'IGNORE   : ', iconv("UTF-8", "ISO-8859-1//IGNORE", $text), PHP_EOL;
echo 'Plain    : ', iconv("UTF-8", "ISO-8859-1", $text), PHP_EOL;
