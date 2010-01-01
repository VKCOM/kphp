@ok
<?php
	$x = 0;
	$y = 0;
	do
	{
		echo "$x\n";
		echo "$y\n";
		$x += $y;
		$y++;
	}
	while ($x < 1000);

	var_dump ($x);
	var_dump ($y);
?>
