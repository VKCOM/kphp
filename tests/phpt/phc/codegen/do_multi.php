@ok
<?php
	$x = 0;
	$y = 0;
	do
	{
		echo "$x\n";
		echo "$y\n";
		do
		{
			$x += $y;
			$y++;
		}
		while ($x < 100);
	}
	while ($x < 1000);

	var_dump ($x);
	var_dump ($y);
?>
