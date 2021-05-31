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
	var_dump ($arr[(int)2.345]);
	var_dump ($arr[(int)true]);
	var_dump ($arr[(int)false]);
	var_dump ($arr["f"]);
?>
