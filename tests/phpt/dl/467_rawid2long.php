@ok
<?php

#ifndef KittenPHP
  function lor ($x, $y) {
    return $x | $y;
  }

  function land ($x, $y) {
    return $x & $y;
  }

  function lshl ($x, $y) {
    return $x << $y;
  }

  function lshr ($x, $y) {
    return $x >> $y;
  }

  function longval ($x) {
    return $x;
  }
#endif

  function dec2hex($dec) {
    bcscale(0);
    $value = '';
    $digits = '0123456789abcdef';
    while ($dec > 15) {
      var_dump($dec);
      var_dump($value);
      $rest = bcmod($dec, 16);
      $dec = bcdiv($dec, 16);
      var_dump($rest);
      $value = $digits[$rest].$value;
    }
    $value = $digits[intval($dec)].$value;
    return (string)$value;
  }

  function rawID2Long($int1, $int2 = false) {
    if ($int2 === false) {
      list($int1, $int2) = explode('_', $int1);
    }
    return lor (lshl ($int1, 32), $int2);
  }

  function long2rawID($num) {
    return lshr ($num, 32).'_'.land($num, longval (0xFFFFFFFF));
  }

  unset ($predicat);
  $a = array ('_type' => $predicat);

  $int1 = '-2345678901';
  $res = str_pad (dec2hex (ltrim ($int1, '-')), 8, '0', STR_PAD_LEFT);
  var_dump ($int1);
  var_dump (dec2hex (ltrim ($int1, '-')));
  var_dump ($res);

  $raw = '-2345678901_2345678901';
  $l = rawID2Long ($raw);
  var_dump ($raw);
  var_dump (strval ($l));
  var_dump (long2rawID($l));
