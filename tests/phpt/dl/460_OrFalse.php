@ok k2_skip
<?php
  fwrite (STDERR, "begin\n");
  $x = "asdasd";
  fwrite (STDERR, "end\n");
exit;

  $x = 1;
  if (0) {
    $x = '';
  }

  $y = false;
  if (0) {
    $y = array();
  }

  $z = $y;
  $y = $x;
  $x = $z;

  if ($z == $y);
  if ($y == $x);
  if ($x == $z);

  if ($z === $y);
  if ($y === $x);
  if ($x === $z);

  if ($z !== $y);
  if ($y !== $x);
  if ($x !== $z);


  $t = false;
  if (0) {
    $t = 1;
  }

  $at = array ($t);
  var_dump ($t !== false);
  var_dump ($at[0] !== false);

//  var_dump ($t);
//  die ($t);


  $config['sms_nosend_prefixes'] = array ("1", "2");
  $phone = "156789";

  foreach ($config['sms_nosend_prefixes'] as $prefix) {
    if (strpos($phone, $prefix) === 0) {
    }
  }

  $a = array(1);

  var_dump (isset ($a[6]));
  var_dump ($a);

  $result = array(1, "");
  var_dump ($result);
  var_dump ($result['_']);
  var_dump ($result['_'] == 'richError');
  var_dump ($result['_'] === 'richError');
  var_dump ($result['_'] !== 'richError');
  var_dump (5 == true);

