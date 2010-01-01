@ok
<?php

	/* String which contain just ints are treated as int keys. This enumerates special cases, which are:
	 *		leading '-' OK, 
	 *		leading '0' not OK,
	 *		decimal only,
	 *		LONG_INT and LONG_MAX excluded
	 */

	$x = array ("0", // int
					"1", // int
					"12345", // int
					"-012345", // not int (leading 0)
					"-12345", // int
					"01", // not int (leading 0)
					"-0", // not int
					"-1", // int
					"-", // not int
//					"", // cb_mir doesnt support this. TODO allow a way of skipping this test for cb_mir, without turning off the CompiledVsInterpreted test
					"123x", // not int
					"-123x", // not int
					"0x", // not int (decimal only)
					"00", // not int (leading 0)
					"-9223372036854775809", // not int (64 bit LONG_MIN - 1)
					"-9223372036854775808", // not int (64 bit LONG_MIN)
					"-9223372036854775807", // int (64 bit LONG_MIN + 1)
					"9223372036854775808", // not int (64 bit LONG_MAX + 1)
					"9223372036854775807", // not int (64 bit LONG_MAX)
					"9223372036854775806", // int (64 bit LONG_MAX - 1)
					"-2147483649", // not int (32 bit LONG_MIN - 1)
					"-2147483648", // int on 64 bit only (32 bit LONG_MIN)
					"-2147483647", // int (32 bit LONG_MIN - 1)
					"2147483648", // not int (32 bit LONG_MAX + 1)
					"2147483647", // int on 64 bit only (32 bit LONG_MAX)
					"2147483646", // int (32 bit LONG_MAX - 1)
					"0x17", // not int (no hex)
					"0xa17" // not int (decimal only)
					);

		foreach ($x as $key)
		{
			$y[$key] = $key;
		}

		foreach ($y as $key => $value)
		{
			print_r ($key);
			var_dump ($value);
		}
?>
