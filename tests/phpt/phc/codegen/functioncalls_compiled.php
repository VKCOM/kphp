@ok
<?php
/*
 * Test of function calls
 *
 * We define two functions fun and fun_r, both of which take a single argument
 * $x; but fun_r is compile-time-by-reference and fun is not.
 *
 * Tests are designed to check copy-on-write etc. stuff
 */

	function fun($x) { $x = 'x'; }
	function fun_r(&$x) { $x = 'x'; }

	// Test proper calling conventions in simple cases

	$a = 10;
	fun($a);
	print_r($a);

	$b = 20;
	fun($b);
	print_r($b);

	$c = 30;
	fun_r($c);
	print_r($c);

	$d = 40;
	fun_r($d);
	print_r($d);

	echo "\n";

	// Same as before, but the variable now passed in is part of a
	// copy-on-write set. In all cases where a pass-by-reference is involved,
	// only one of the two variables should be changed.

	$e = 50;
	$f = $e;
	fun($f);
	print_r($e);
	print_r($f);
	$f = 'y';
	print_r($e);
	print_r($f);
	
	$g = 60;
	$h = $g;
	fun($h);
	print_r($g);
	print_r($h);
	$h = 'y';
	print_r($g);
	print_r($h);

	$i = 70;
	$j = $i;
	fun_r($j);
	print_r($i);
	print_r($j);
	$j = 'y';
	print_r($i);
	print_r($j);

	$k = 80;
	$l = $k;
	fun_r($l);
	print_r($k);
	print_r($l);
	$l = 'y';
	print_r($k);
	print_r($l);
	
	echo "\n";

	// Same again, but the variable passed in is now part of a 
	// change-on-write set (references another variable). 
	// In this section, in all cases where a pass-by-reference is involved,
	// both variables should be changed.

	$m = 90;
	$n = $m;
	fun($n);
	print_r($m);
	print_r($n);
	$m = 'y';
	print_r($m);
	print_r($n);

	$o = 100;
	$p = $o;
	fun($p);
	print_r($o);
	print_r($p);
	$p = 'y';
	print_r($o);
	print_r($p);

	$q = 110;
	$r = $q;
	fun_r($r);
	print_r($q);
	print_r($r);
	$r = 'y';
	print_r($q);
	print_r($r);

	$s = 120;
	$t = $s;
	fun_r($t);
	print_r($s);
	print_r($t);
	$t = 'y';
	print_r($s);
	print_r($t);

	echo "\n";
?>
