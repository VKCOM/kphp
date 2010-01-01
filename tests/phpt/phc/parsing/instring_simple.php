@ok
<?php
	$a = $b = $c = 1;
	echo "foo $a bar\n";
	echo "foo " . $a . " bar\n";
	echo "1 $a 2" . "3 $b 4\n";
	echo "1 $a 2" . $b . "3 $c 4\n";
	echo "1 $a 2" . $b . $c . "3 $c 4\n";
?>
