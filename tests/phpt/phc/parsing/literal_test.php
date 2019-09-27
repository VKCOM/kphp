@ok
<?php
	echo "0x7ffffffd\t= "; var_dump(0x7ffffffd);
	echo "0x7ffffffe\t= "; var_dump(0x7ffffffe);
	echo "0x7fffffff\t= "; var_dump(0x7fffffff);
	
	echo "2147483645\t= "; var_dump(2147483645);
	echo "2147483646\t= "; var_dump(2147483646);
	echo "2147483647\t= "; var_dump(2147483647);


	echo "-0x7ffffffd\t= "; var_dump(-0x7ffffffd);
	echo "-0x7ffffffe\t= "; var_dump(-0x7ffffffe);
	echo "-0x7fffffff\t= "; var_dump(-0x7fffffff);
	echo "-0x80000000\t= "; var_dump(-0x80000000);
	
	echo "-2147483645\t= "; var_dump(-2147483645);
	echo "-2147483646\t= "; var_dump(-2147483646);
	echo "-2147483647\t= "; var_dump(-2147483647);
	echo "-2147483648\t= "; var_dump(-2147483648);

?>
