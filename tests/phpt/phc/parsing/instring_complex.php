@ok
<?php
	$x = 1;
	$y["foo"] = 2;
	$y[3] = 3;
	$y[1][2] = 4;
	$z = 3;

	echo "a {$x} b\n";
	echo "c {$y["foo"]} d\n";
	echo "e {$y[1+2]} f\n";
	echo "g {$y[$z]} h\n";
	echo "i {$y[1][2]} j\n";
	echo "{$y[1][2]} k\n";

	echo "a ${x} b\n";
	echo "a${x}b\n";
	echo "${x}${z}\n";
?>
