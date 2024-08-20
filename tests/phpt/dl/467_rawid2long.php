@ok k2_skip
<?php

  function dec2hex($dec) {
    bcscale(0);
    $value = '';
    $digits = '0123456789abcdef';
    while ($dec > 15) {
      var_dump($dec);
      var_dump($value);
      $rest = (int)bcmod($dec, 16);
      $dec = bcdiv($dec, 16);
      var_dump($rest);
      $value = $digits[$rest].$value;
    }
    $value = $digits[intval($dec)].$value;
    return (string)$value;
  }

  function rawID2int($int1, $int2 = false) {
    if ($int2 === false) {
      list($int1, $int2) = explode('_', $int1);
    }
    return ($int1 << 32) | $int2;
  }

  /**
   * @param int $num
   * @return string
   */
  function int2rawID($num) {
    return ($num >> 32).'_'.($num & 0xFFFFFFFF);
  }

  unset ($predicat);
  $a = array ('_type' => $predicat);

  $int1 = '-2345678901';
  $res = str_pad (dec2hex (ltrim ($int1, '-')), 8, '0', STR_PAD_LEFT);
  var_dump ($int1);
  var_dump (dec2hex (ltrim ($int1, '-')));
  var_dump ($res);

  $raw = '-2345678901_2345678901';
  $l = rawID2int ($raw);
  var_dump ($raw);
  var_dump (strval ($l));
  var_dump (int2rawID($l));
