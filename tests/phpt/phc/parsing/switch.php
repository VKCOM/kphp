@ok
<?php
  $argv = array (1);
	$x = $argv[1];
	
	switch($x)
	{
		case 1:
			echo "one\n";
			break;
		case 2:
			echo "two\n";
			break;
		default:
			echo "unknown\n";
			break;
	}

	switch($x)
	{
		default:
			echo "unknown\n";
			break;
		case 1:
			echo "one\n";
			break;
		case 2:
			echo "two\n";
			break;
	}

	switch(1)
	{
		case 2:
		
	}
/*
	switch(3):
		case 1:
			1.1;
		case 2:
			2.2;
	endswitch;*/
?>
