@ok
<?php

function filesBinFromBase64($str) { // for storage-engine: secret packing
  return base64_decode(strtr($str, '-_', '+/').str_repeat('=', ((65536 - strlen($str)) % 4)));
}

function filesBinToBase64($str) { // for storage-engine: secret packing
  return rtrim(strtr(base64_encode($str), '+/', '-_'), '=');
}

function filesHashEncodedBase64($val, $key) {
  $val_arg = $val;
  $val = $val . substr(md5($val), 0, 12);
  var_dump("val=$val_arg | key=$key | new_val=$val");
  $len = strlen($val);
  $count = ceil($len/16);
  $hash = '';
  for ($i = 0; $i < $count; $i++) {
    $hash .= md5("jEc#0n{$key}9I^Ec@G2d{$i}i9IE");
  }
  var_dump ($hash);
  for ($i = 0; $i < $len; $i++) {
    if ($i == 45) {
      var_dump ($val[$i], ord($val[$i]), substr($hash, 2*$i, 2), hexdec(substr($hash, 2*$i, 2)), ord($val[$i]) ^ hexdec(substr($hash, 2*$i, 2)));
      var_dump ("-----------------");
      var_dump (ord($val[$i]) ^ hexdec(substr($hash, 2*$i, 2)));
      var_dump (chr (0));
      var_dump (chr(ord($val[$i]) ^ hexdec(substr($hash, 2*$i, 2))));
    }
    $val[$i] = chr(ord($val[$i]) ^ hexdec(substr($hash, 2*$i, 2)));
  }
  var_dump (bin2hex ($val));
  return filesBinToBase64($val);
}

function filesHashDecodedBase64($val, $key) {
  $val = filesBinFromBase64($val);
  $len = strlen($val);
  $count = ceil($len/16);
  $hash = '';
  for ($i = 0; $i < $count; $i++) {
    $hash .= md5("jEc#0n{$key}9I^Ec@G2d{$i}i9IE");
  }
  for ($i = 0; $i < $len; $i++) {
    $val[$i] = chr(ord($val[$i]) ^ hexdec(substr($hash, 2*$i, 2)));
  }
  $end = substr($val, -12);
  $val = substr($val, 0, -12);
  if (substr(md5($val), 0, 12) != $end) {
    return '';
  }
  return $val;
}

function filesEncodeAudioExtra($extra_data, $source, $uid) {
  $extra = '';
  foreach($extra_data as $key => $val) {
    $extra = $extra . ($extra ? ',' : '') . "$key=$val";
  }
  $key = md5("E5j{$source}6#78d@E{$uid}5hb^dx4");
  var_dump ($extra);
  return filesHashEncodedBase64($extra, $key);
}

function filesDecodeAudioExtra($extra, $source, $uid) {
  $key = md5("E5j{$source}6#78d@E{$uid}5hb^dx4");
  $extra = filesHashDecodedBase64($extra, $key);
  $pairs = explode(',', $extra);
  $extra_data = array();
  foreach($pairs as $pair) {
    if (preg_match('/^([a-zA-Z0-9]+)=([a-zA-Z0-9\.:-]+)$/', $pair, $matches)) {
      $extra_data[$matches[1]] = $matches[2];
    }
  }
  return $extra_data;
}

$extra_data = array(
't' => 'd',
'x' => '168181945',
'o' => '175277028',
'c' => 3,
'd' => 1378978200,
'i' => '85.76.145.124',
);
$source = 'fb3abb5b835e';
$mid = 189814;
$extra = filesEncodeAudioExtra($extra_data, $source, $mid);
$res = filesDecodeAudioExtra($extra, $source, $mid);

var_dump ($extra, $res);

$s_zero = ' ';

var_dump ($s_zero);

$s_zero = '12345';

var_dump ($s_zero);

$s_zero = ' ';
$s_zero[1] = '\0';

var_dump ($s_zero);

$s_zero = '12345';
$s_zero[1] = '\0';

var_dump ($s_zero);

$s_zero = ' ';
$s_zero[1] = "\x00";

var_dump ($s_zero);

$s_zero = '12345';
$s_zero[1] = "\x00";

var_dump ($s_zero);


$s_zero = ' ';

var_dump ($s_zero);

$s_zero = '12345';

var_dump ($s_zero);

$s_zero = ' ';
$s_zero[10] = '\0';

var_dump ($s_zero);

$s_zero = '12345';
$s_zero[10] = '\0';

var_dump ($s_zero);

$s_zero = ' ';
$s_zero[10] = "\x00";

var_dump ($s_zero);

$s_zero = '12345';
$s_zero[10] = "\x00";

var_dump ($s_zero);

