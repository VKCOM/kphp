@ok
<?php

function md5LastBits($x) {
  $x = md5($x);
  $l = strlen($x);
  $x = substr($x, $l < 3 ? 0 : $l - 3, 3);
  return hexdec($x);
}


function ipv6ToArray($ipv6) {
  $ipv6 = strtolower($ipv6);
  if (substr($ipv6, 0, 2) == '::') {
    $ipv6 = substr($ipv6, 1, strlen($ipv6) - 1);
  } else {
    $len = strlen($ipv6);
    if ($len >= 2 && substr($ipv6, $len - 2, 2) == '::') {
      $ipv6 = substr($ipv6, 0, $len - 1);
    }
  }
  $x = explode(':', $ipv6);
  $l = count($x);
  if ($l < 2 || $l > 8) {
    return false;
  }
  if ($l < 8) {
    $k = -1;
    for ($i = 0; $i < $l; $i++) {
      echo $k;
      var_dump ('i = '.$i.', k = '.$k);
      if ($x[$i] == '') {
        if ($k >= 0) {
          return false;
        }
        $k = $i;
      }
    }
    var_dump ($k);
    if ($k < 0) {
      return false;
    }
    $t = array();
    for ($i = 0; $i < $k; $i++) {
      $t[] = $x[$i];
    }
    for ($i = 0; $i < 8 - $l; $i++) {
      $t[] = '0000';
    }
    for ($i = $k; $i < $l; $i++) {
      $t[] = $x[$i];
    }
    $x = $t;
  }
  for ($i = 0; $i < 8; $i++) {
    var_dump('a' . $x[$i]);
    $d = strlen($x[$i]);
    var_dump('b' . $x[$i]);
    if ($d > 4) {
      return false;
    }
    var_dump("i = $i");
    var_dump($d);
    while ($d < 4) {
      var_dump($x[$i]);
      $x[$i] = '0'.$x[$i];
      ++$d;
      var_dump($d);
      var_dump($x[$i]);
    }
  }
  return $x;
}


function ipv6ToIpv4($ipv6) {
  $x = ipv6ToArray($ipv6);
  if (!$x) {
    return '255.255.255.255';
  }
  $f1 = md5LastBits($x[0].$x[1]);
  $f2 = md5LastBits($x[2].$x[3]);
  $f3 = md5LastBits($x[4].$x[5].$x[6].$x[7]);
  $n1 = (($f1 >> 4) & 0x1f) | 0xe0;
  $n2 = (($f1 & 0x0f) << 4) | ($f2 >> 8);
  $n3 = $f2 & 0xff;
  $n4 = $f3 & 0xff;
  return $n1.".".$n2.".".$n3.".".$n4;
}

function ipv6Normalize($ipv6) {
  $x = ipv6ToArray($ipv6);
  if (!$x) {
    return false;
  }
  return $x[0].":".$x[1].":".$x[2].":".$x[3].":".$x[4].":".$x[5].":".$x[6].":".$x[7];
}

var_dump (ipv6Normalize ("2001:0:5ef5:79fd:24ec:2b3c:af01:e85d"));
var_dump (ipv6ToIpv4 ("2001:0:5ef5:79fd:24ec:2b3c:af01:e85d"));
var_dump (ipv6ToIpv4 ("2001:0::2b3c:af01:e85d"));
var_dump (ipv6ToIpv4 ("2001:0:5ef5:79fd:24ec:2b3c::"));
var_dump (ipv6ToIpv4 (":2001:0:5ef5:79fd:24ec:2b3c:"));
var_dump (ipv6ToIpv4 ("::2001:0:5ef5:79fd:24ec:2b3c"));
var_dump (ipv6ToIpv4 ("::::::::"));
