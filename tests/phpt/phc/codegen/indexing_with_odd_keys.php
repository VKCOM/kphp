@ok
<?php
	$arr = array(
		10 => "a",
		2.345 => "b",
		true => "c",
		false => "d",
		NULL => "e",
		"f" => "g");
	
	var_dump($arr);

	var_dump ($arr[10]);
	var_dump ($arr[2.345]);
	var_dump ($arr[true]);
	var_dump ($arr[false]);
	var_dump ($arr[NULL]);
	var_dump ($arr["f"]);
?>
