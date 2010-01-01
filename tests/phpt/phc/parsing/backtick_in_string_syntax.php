@unsupported
<?php
	// Backticked strings support in-string 
	$root = "/";
	echo `ls $root`;

	// and escaping
	echo `ls \057`;

	// and the backtick itself can be escaped
	echo `ls \\\``;
?>
