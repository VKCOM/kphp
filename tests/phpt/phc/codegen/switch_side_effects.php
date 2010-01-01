@ok
<?php

	function f () { echo "1\n"; return 7; }

	switch (f())
	{
		case 1:
			echo "one\n";
			break;
		case 2:
			echo "two\n";
			break;
		case 3:
			echo "three\n";
			break;
		case 4:
			echo "four\n";
			break;
		case 5:
			echo "five\n";
			// fallthrough
		case 6:
			echo "six\n";
			// fallthrough
		case 7:
			echo "seven\n";
			// fallthrough
		default:
			echo "default\n";
	}

?>
