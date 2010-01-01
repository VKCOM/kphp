@ok no_php
<?php

	x ();
	// The braces move it out of the list of functions which are read on script
	// loading. This could be handled by translating to if (true) { ... }
	{
		function x () { print "Called\n"; }
	}

?>
