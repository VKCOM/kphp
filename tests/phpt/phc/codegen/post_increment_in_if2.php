@ok
<?php
	$i = 0;
	if($i++ == 0)
	{
		$j = 0;
		if($j++ == 0)
			echo "(a,a) $i $j\n";
		else
			echo "(a,b) $i $j\n";
	}
	else
	{
		echo "(b) $i\n";
	}
?>
