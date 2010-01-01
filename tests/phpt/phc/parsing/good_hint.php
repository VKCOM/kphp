@unsupported
<?php

	class A {}

	// All these hints should be OK
	function x1 (int $y) {}
	function x2 (bool $y) {}
	function x3 (array $y) {}
	function x4 (string $y) {}
	function x5 (A $y) {}

	// hints with OK default values
	function y1 (int $y = NULL) {}
	function y2 (bool $y = NULL) {}
	function y3 (array $y = NULL) {}
	function y3_1 (array $y = array (array (7))) {}
	function y4 (string $y = NULL) {}
	function y5 (A $y = NULL) {}

?>
