@ok
<?php

$a = array (1, 3, 4);
foreach ($a as $x) {
  foreach ($a as $x) {
    continue 2;
  }

  foreach ($a as $x) {
  }
}

//function convertToJSON($x) {
  //return "JSON";
//}

//$config['to_json'] = 'convertToJSON';
//var_dump ($config['to_json'](123));
//var_dump ("test it {$config['to_json'](123)}");
//class A{
//};


/*
function test2() {
  $a_array = array (3, 1, 2);
  sort ($a_array);
  var_dump ($a_array);

  $a_var = false;
  $a_var = array (3, 1, 2);
  sort ($a_var);

  $a_var_int = false;
  $a_var_int = 123;

  sort ($a_var_int);

  
  var_dump ("-------------");
  $a_int = 123;
  str_replace ('a', 'b', 'aaaa', $a_var);
  var_dump ($a_var);
  str_replace ('a', 'b', 'aaaa', $a_var_int);
  var_dump ($a_var_int);
  str_replace ('a', 'b', 'aaaa', $a_int);
  var_dump ($a_int);
}
var_dump ("test2");
test2();
*/

/*function test3() {
  print "abc\0def" == "abc";
  print "\n";
}

test3();*/ //Works now

/*function test4() {
  $a = array (new Memcache());
  $a[0]->get ('a');
  if (!$a[0]->get ('a') || !$a[0]->get ('b')) {
    echo "ok\n";
  }
}

test4();*/
