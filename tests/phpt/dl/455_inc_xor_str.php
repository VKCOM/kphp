@ok
<?php
  function inc_xor_str($a, $b) {
    $len = strlen($a);
    for ($i = 0; $i < $len; $i++) {
      $a[$i] = dechex(hexdec($a[$i]) ^ hexdec($b[$i]));
    }
    return $a;
  }

  $a = "38";
  $b = "37";

  var_dump (inc_xor_str ($a, $b));
  var_dump ($a);
  var_dump ($b);

  var_dump (inc_xor_str ("38", "37"));

  var_dump ($a[0]);
  var_dump ($a[1]);
  var_dump ($a[2]);
  var_dump ($a[12]);
  @var_dump ($a[""]);
  @var_dump ($a["asdas"]);
  @var_dump ($a["1asd"]);
  @var_dump ($a["12asd"]);

  var_dump (isset ($a[0]));
  var_dump (isset ($a[1]));
  var_dump (isset ($a[2]));
  var_dump (isset ($a[12]));
  @var_dump (isset ($a[""]));
  @var_dump (isset ($a["asdas"]));
  @var_dump (isset ($a["1asd"]));
  @var_dump (isset ($a["12asd"]));

  $a[0] = 'z';
  var_dump ($a);
  $a[1] = 'z';
  var_dump ($a);
  $a[2] = 'z';
  var_dump ($a);
  $a[12] = 'z';
  var_dump ($a);
  @$a[""] = 'y';
  var_dump ($a);
  @$a["asdas"] = 'y';
  var_dump ($a);
  @$a["1asd"] = 'y';
  var_dump ($a);
  @$a["12asd"] = 'y';
  var_dump ($a);
