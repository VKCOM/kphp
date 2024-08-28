@ok k2_skip
<?php
  $a = array (-1 => 1, 1 => false, 3 => null);
  $b = array (1, false, null);

  for ($i = -1; $i < 5; $i++) {
    var_dump (isset ($a[$i]));
    var_dump (isset ($b[$i]));
  }

  var_dump (isset ($a['']));
  var_dump (isset ($b['']));

  var_dump (isset ($a['1 day']));
  var_dump (isset ($b['1 day']));

  var_dump (isset ($a[1.5]));
  var_dump (isset ($b[1.5]));

  $a = array (-1 => 1, 1 => false, 3 => null);
  $b = array (1, false, null);

  for ($i = -1; $i < 5; $i++) {
    var_dump (array_key_exists ($i, $a));
    var_dump (array_key_exists ($i, $b));
  }

  var_dump (array_key_exists ('', $a));
  var_dump (array_key_exists ('', $b));

  var_dump (array_key_exists ('1 day', $a));
  var_dump (array_key_exists ('1 day', $b));

  $c = array (0);
  var_dump (isset($c[0.0])); //true
  var_dump (array_key_exists(0, $c)); //false



