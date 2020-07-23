@ok
<?php
$a = 0;
/**
 * @kphp-infer
 * @param mixed $x
 */
function f($x) {
  global $a;
  switch ($x) {
    case $a:
    case 1:
      echo "first\n";
    case "1":
      echo "second\n";
      break;
    default:
      echo "default\n";
    case "2":
      echo "third\n";
    case 2:
      echo "forth\n";
  }  
}
f(0);
f(1);
f("1");
f(2);
f("2");
f(3);

/**
 * @kphp-infer
 * @param string $x
 */
function g($x) {
  switch ($x) {
  case 'abc':
    echo "abc";
    break;
  case '':
    echo "empty";
    break;
  }
}

g ('');
