@unsupported
<?php
	class C {}

	$x = new C();
	$y = "C";

	$z1 = clone($x);
	$z2 = clone($y);
?>
