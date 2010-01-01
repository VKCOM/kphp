@todo
<?php

	echo "\nHexdecimal ints near LONG_MAX (automatic conversion to float after LONG_MAX)\n";
	echo "\nIn 5.2.1, these behave just like decimal ints\n";
	echo "0x7ffffffd\t= "; var_dump(0x7ffffffd);
	echo "0x7ffffffe\t= "; var_dump(0x7ffffffe);
	echo "0x7fffffff\t= "; var_dump(0x7fffffff);
	echo "0x80000000\t= "; var_dump(0x80000000);
	echo "0x80000001\t= "; var_dump(0x80000001);
	echo "0x80000002\t= "; var_dump(0x80000002);
	
	echo "\nDecimal ints near LONG_MAX (automatic conversion to float after LONG_MAX)\n";
	echo "2147483645\t= "; var_dump(2147483645);
	echo "2147483646\t= "; var_dump(2147483646);
	echo "2147483647\t= "; var_dump(2147483647);
	echo "2147483648\t= "; var_dump(2147483648);
	echo "2147483649\t= "; var_dump(2147483649);
	echo "2147483650\t= "; var_dump(2147483650);

	echo "\nHexdecimal ints near ULONG_MAX(truncated to int(LONG_MAX) after ULONG_MAX)\n";
	echo "\nIn 5.2.1, these behave just like decimal ints\n";
	echo "0xfffffffd\t= "; var_dump(0xfffffffd);
	echo "0xfffffffe\t= "; var_dump(0xfffffffe);
	echo "0xffffffff\t= "; var_dump(0xffffffff);
	echo "0x100000000\t= "; var_dump(0x100000000);
	echo "0x100000001\t= "; var_dump(0x100000001);
	echo "0x100000002\t= "; var_dump(0x100000002);

	echo "\nDecimal ints near ULONG_MAX(floats as far as the eye can see)\n";
	echo "4294967293\t= "; var_dump(4294967293);
	echo "4294967294\t= "; var_dump(4294967294);
	echo "4294967295\t= "; var_dump(4294967295);
	echo "4294967296\t= "; var_dump(4294967296);
	echo "4294967297\t= "; var_dump(4294967297);
	echo "4294967298\t= "; var_dump(4294967298);

	echo "\nHexdecimal ints near -LONG_MAX (automatic conversion to float after LONG_MAX)\n";
	echo "\nIn 5.2.1, these behave just like decimal ints\n";
	echo "-0x7ffffffd\t= "; var_dump(-0x7ffffffd);
	echo "-0x7ffffffe\t= "; var_dump(-0x7ffffffe);
	echo "-0x7fffffff\t= "; var_dump(-0x7fffffff);
	echo "-0x80000000\t= "; var_dump(-0x80000000);
	echo "-0x80000001\t= "; var_dump(-0x80000001);
	echo "-0x80000002\t= "; var_dump(-0x80000002);
	
	echo "\nDecimal ints near -LONG_MAX (automatic conversion to float after LONG_MAX)\n";
	echo "-2147483645\t= "; var_dump(-2147483645);
	echo "-2147483646\t= "; var_dump(-2147483646);
	echo "-2147483647\t= "; var_dump(-2147483647);
	echo "-2147483648\t= "; var_dump(-2147483648);
	echo "-2147483649\t= "; var_dump(-2147483649);
	echo "-2147483650\t= "; var_dump(-2147483650);

	echo "\nHexdecimal ints near -ULONG_MAX(truncated to int(LONG_MAX) after ULONG_MAX)\n";
	echo "-0xfffffffd\t= "; var_dump(-0xfffffffd);
	echo "-0xfffffffe\t= "; var_dump(-0xfffffffe);
	echo "-0xffffffff\t= "; var_dump(-0xffffffff);
	echo "-0x100000000\t= "; var_dump(-0x100000000);
	echo "-0x100000001\t= "; var_dump(-0x100000001);
	echo "-0x100000002\t= "; var_dump(-0x100000002);

	echo "\nDecimal ints near -ULONG_MAX(floats as far as the eye can see)\n";
	echo "-4294967293\t= "; var_dump(-4294967293);
	echo "-4294967294\t= "; var_dump(-4294967294);
	echo "-4294967295\t= "; var_dump(-4294967295);
	echo "-4294967296\t= "; var_dump(-4294967296);
	echo "-4294967297\t= "; var_dump(-4294967297);
	echo "-4294967298\t= "; var_dump(-4294967298);


?>
