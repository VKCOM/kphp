@unsupported
<?php

	// Try reflection using each scalar type. This allows us to write to a
	// number of variables which wouldn't otherwise be accessible, since they
	// cannot be parsed.

	echo "\n\nasd:\n";
	echo "\n\n5:\n";
	echo "\n\n-5:\n";

	var_dump ($asd);
	var_dump (${5});
	var_dump (${-5});
	var_dump (${true});
	var_dump (${false});
	var_dump (${5.7});
	var_dump (${NULL});

	${"asd"} = 6;
	${5} = 1;
	${-5} = 1;
	${true} = 2;
	${false} = 3;
	${5.7} = 4;
	${NULL} = 5;

	var_dump ($asd);
	var_dump (${5});
	var_dump (${-5});
	var_dump (${true});
	var_dump (${false});
	var_dump (${5.7});
	var_dump (${NULL});

	// TODO how to read the symbol table
?>
