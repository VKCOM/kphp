@ok
<?php
  $argc = 0;

	if ($argc == 0)
	{
		$value = 1;
		$type = 2;
		$solo = 1;
		$various = array (6);
	}
	else if ($argc == 3)
	{
		$value = 1;
		$type = "string";
		$various = 1;
	}
	else
	{
	}

	var_dump ($type);
	var_dump ($value);
	var_dump ($various);

?>
