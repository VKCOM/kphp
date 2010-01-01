@ok
<?php
	echo "123";
	echo "123 \001";
	echo "123 \001 \002 \003 \004 \005 \006 456";
	//originallyt there was null character
	echo "123 \001 \001 \002 \003 \004 \005 \006 456";	
	echo "123 \001";

	echo <<<EOS
123 \001 \001 \002 \003 \004 \005 \006 456
EOS;
?>
