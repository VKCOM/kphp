@ok
<?php
  $x = 0;
  $y = 0;
  $z = 0;
	while ($x < 10)
	{
		echo "$x\n";
		while ($y < 20)
		{
			echo "$y\n";
			$y++;
			$x++;
		}
		$z++;
		echo "$z\n";
	}

	var_dump ($x);
	var_dump ($y);
	var_dump ($z);
?>
