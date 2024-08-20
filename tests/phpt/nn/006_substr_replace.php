@ok k2_skip
<?php

function substr_replace_test1() {
  $var = 'ABCDEFGH:/MNRPQR/';
  echo "Оригинал: $var<hr />\n";

  /* Обе следующих строки заменяют всю строку $var на 'bob'. */
  echo substr_replace($var, 'bob', 0) . "<br />\n";
  echo substr_replace($var, 'bob', 0, strlen($var)) . "<br />\n";

  /* Вставляет 'bob' в начало $var. */
  echo substr_replace($var, 'bob', 0, 0) . "<br />\n";

  /* Обе следующих строки заменяют 'MNRPQR' в $var на 'bob'. */
  echo substr_replace($var, 'bob', 10, -1) . "<br />\n";
  echo substr_replace($var, 'bob', -7, -1) . "<br />\n";

  /* Удаляет 'MNRPQR' из $var. */
  echo substr_replace($var, '', 10, -1) . "<br />\n";
}

function substr_replace_test2() {
  var_dump(substr_replace('eggs','x',-1,-1)); //eggxs 
  var_dump(substr_replace('eggs','x',-1,-2)); //eggxs 
  var_dump(substr_replace('eggs','x',-1,-2)); //eggxs 
  var_dump(substr_replace('eggs','x',-1,0)); //eggxs 
  var_dump(substr_replace('huevos','x',-2,-2)); //huevxos 
  var_dump(substr_replace('huevos','x',-2,-3)); //huevxos 
  var_dump(substr_replace('huevos','x',-2,-3)); //huevxos 
  var_dump(substr_replace('huevos','x',-2,0)); //huevxos 
  var_dump(substr_replace('abcd', 'x', 0, -4)); //xabcd 
  var_dump(substr_replace('abcd','x',0,0)); //xabcd 
  var_dump(substr_replace('abcd', 'x', 1, -3)); //axbcd 
  var_dump(substr_replace('abcd', 'x', 1, 0)); //axbcd 
}

$s = "fdf";
$t = "abacaba";
$s1 = substr_replace($s, $t, 0, 1);
$s2 = substr_replace($s1, $t, -1);
var_dump($s);
var_dump($s1);
var_dump($s2);

substr_replace_test1();
substr_replace_test2();
