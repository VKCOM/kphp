@ok pass
<?php

	$scalar_array = array (
		"",
		"a",
		"ab",
		"aasd",
		"really quite a really really long string, longer even",
		array (),
		array (17, 45),
		array (17 => "asd", 45 => ""),
		array ("ppp", "jjj"),
		array ("ppp" => 17, "jjj" => 45),
		array ("2", "3"),
		array ("2" => 2, "3" => 3),
		array (array ("rrr")),
		56,
		-17,
		0,
		1,
		-1,
		62.75,
		0.0,
		-0.0,
		NULL,
		false,
		true);

	$short_scalar_array = array (
		"",
		"really quite a re",
		array ("2", 3, "four"),
		0,
		1,
		-1,
		62.75,
		0.0,
		-0.0,
		NULL,
		false,
		true);

?>
