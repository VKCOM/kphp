@ok
<?php


/**
 * @kphp-infer
 * @param string[] $a
 */
function f($a = array ("a" => "b")) {
  print_r ($a);
  print "\n";
}
f();

$a = md5 ("a". "b");
print ($a);

array_filter (array ($a));

