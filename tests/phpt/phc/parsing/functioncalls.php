@ok
<?php
	function f() {}
/**
 * @kphp-infer
 * @param int|null $a
 */
	function g($a) {}
/**
 * @kphp-infer
 * @param int|null $a
 * @param int|null $b
 */
	function h($a, $b) {}
/**
 * @kphp-infer
 * @param int|null $a
 * @param int|null $b
 * @param int|null $c
 */
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
