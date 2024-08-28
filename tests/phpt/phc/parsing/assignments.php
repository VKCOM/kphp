@ok k2_skip
<?php
  unset ($b);
	$a = $b;
#	$a = &$b;

	$x = 10;
	$y = 20;
	$z = 33;

	$x += $y;
	var_dump ($x);
	$x -= $y;
	var_dump ($x);
	$x *= $y;
	var_dump ($x);
	$x /= $z;
	var_dump ($x);
	@$x %= $y;
	var_dump ($x);

	$x .= $y;
	var_dump ($x);

	$x &= $y;
	var_dump ($x);
	$x |= $y;
	var_dump ($x);
	$x ^= $y;
	var_dump ($x);

	$x <<= $y;
	var_dump ($x);
	$x >>= $y;
	var_dump ($x);

	$x++;
	var_dump ($x);
	++$x;
	var_dump ($x);
	$x--;
	var_dump ($x);
	--$x;
	var_dump ($x);
?>
