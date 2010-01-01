@unsupported
<?php

# files from the current working directory

$x = array(1 => "a", 2 => "b", 3 => "c");


$xx = "x";
unset($y);

echo "** 9 **\n";
foreach ($$xx as $y => &$z) {
	echo "" . $y . " => " . $z . "\n";
}

var_dump($x);
var_dump($y);
var_dump($xx);

?>
