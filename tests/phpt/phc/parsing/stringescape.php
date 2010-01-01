@ok
<?php
/* test octal */
	echo "\100\n";
	
/* test hex */
	echo "\x40\n";

/* test non-printable characters */
	echo "Some non-printable chars: \001 \001 \x02 \x7F\n";	

/* other escaping, both in DQ_STR and HD_STR */
	echo "a\nb";
	echo<<<END
a\nb
END;
	echo "a\"b";
	echo <<<END
a\"b
END;

?>
