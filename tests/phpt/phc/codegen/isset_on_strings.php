@ok
<?php
	// isset on strings 
	$x = "012";
	var_dump(isset($x{-1}));
	var_dump(isset($x{0}));
	var_dump(isset($x{1}));
	var_dump(isset($x{2}));
	var_dump(isset($x{3}));
	var_dump(isset($x{4}));
?>
