@unsupported
<?php

	// and post-op
	$str = "asdasd";
	var_dump ($str++);
	var_dump ($str++);
	var_dump ($str++);
	var_dump ($str++);


	$str = "0as0dasd0";
	var_dump ($str++);
	var_dump ($str++);
	var_dump ($str++);
	var_dump ($str++);

	$str = "10";
	var_dump ($str++);
	var_dump ($str++);
	var_dump ($str++);
	var_dump ($str++);

	// and pre-op
	$str = "asdasd";
	var_dump (++$str);
	var_dump (++$str);
	var_dump (++$str);
	var_dump (++$str);


	$str = "0as0dasd0";
	var_dump (++$str);
	var_dump (++$str);
	var_dump (++$str);
	var_dump (++$str);

	$str = "10";
	var_dump (++$str);
	var_dump (++$str);
	var_dump (++$str);
	var_dump (++$str);



?>
