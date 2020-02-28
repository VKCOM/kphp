@ok
<?php
#ifndef KittenPHP
var_dump([]);
exit;
?>
#endif

  /** @var mixed[] $x */
  $x = [];

	// Illustrates the problem of turning $x[0] into $t =& $x[0]
	$x[0];
	var_dump ($x);

?>
