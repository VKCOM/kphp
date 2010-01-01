@ok
<?php
	// before 1
	// before 2
	echo "hello world"; // after

	if (true)//if($cond)
	{
		// a comment 1
		// a comment 2
		echo "foo";
		// and another comment 1 
		// and another comment 2
	}

	if (true)//if($cond)
	{
		// comment in an empty block 1
		// comment in an empty block 2
	}

	// Comment before the function 1
	// Comment before the function 2
	function f()
	{
		// Comment in an empty function 1
		// Comment in an empty function 2
	}

	echo "booh"; // after booh
	// very last one
?>
