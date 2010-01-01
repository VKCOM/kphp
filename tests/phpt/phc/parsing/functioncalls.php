@ok
<?php
	function f() {}
	function g($a) {}
	function h($a, $b) {}
	function i($a, $b, $c) {}

	f();

	$x = $y = $z = null;

	g(5);
	g($x);
#	g(&$x);

	h(5, 6);
	h($x, $y);
#	h(&$x, &$y);

	i(5, 6, 7);
	i($x, $y, $z);
#	i(&$x, &$y, &$z);
?>
