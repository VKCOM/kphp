@ok
<?php
	$stringNonEmpty = "foo";
	$stringEmpty = "";
	$stringZero = "0";

	$intNonZero = 1;
	$intZero = 0;
	
	$null = NULL;

	$boolTrue = true;
	$boolFalse = false;

	$arrayNonEmpty = array(1); 
	unset ($arrayEmpty);
	$arrayEmpty = array();

  unset ($notSet);
	var_dump(empty($notSet));

	var_dump(empty($stringNonEmpty));
	var_dump(empty($stringEmpty));
	var_dump(empty($stringZero));

	var_dump(empty($intNonZero));
	var_dump(empty($intZero));
	
	var_dump(empty($null));

	var_dump(empty($boolTrue));
	var_dump(empty($boolFalse));

	var_dump(empty($arrayNonEmpty));
	var_dump(empty($arrayEmpty));
?>
