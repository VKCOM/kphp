@ok
<?php
  unset ($a);
  unset ($b);
  unset ($c);
  unset ($d);
	$query1 = 
		" SELECT " . $a . 
		" FROM " . $b .
		" WHERE " . $c . " GROUPBY " . $d;

	$query2 =
		  " SELECT " . $a
		. " FROM " . $b
		. " WHERE " . $c . " GROUPBY " . $d;

	echo $query1; echo $query2; 
?>
