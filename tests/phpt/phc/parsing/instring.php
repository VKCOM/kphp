@ok
<?php
	define("sc", 0);
	
	class X {
		public $y;

		function __construct() {
			$this->y = 123;
		}
	}

	$b = 1;
	$arr[0] = "foo";
	$arr["sc"] = "boo";
	$arr[-1] = "negative";
	$str = "bar";
	$x = new X();
	
	/*
	 * Simple syntax
	 *
	 * This might actually turn out to be more difficult to parse than
	 * "complex" syntax (which is standard syntax enclosed in {..}, where
	 * the first "{" must be followed by a "$".
	 *
	 * Possible variables:
	 *   "$a"     a an ident
	 *   "${a}"   a an ident
	 *   "$a->b"  a, b an ident
	 *   "$a[b]"  b an integer
	 *   "$a[-b]" b an integer (since PHP 7.1)
	 *   "$a[b]"  b an ident (interpreted as a string)
	 *   "$a[$b]" a, b and ident
	 */

  unset ($bc);
	echo "a $bc\n";
  echo "a ${b}c\n";
  echo "a $arr[0] c\n";
  echo "a $arr[-1] c\n";
  echo "a $arr[$b] c\n";
  echo "a $arr[sc] c\n"; 		// "a " . $b["x"] . " c\n"; prints "boo"
  echo "a $str{0} c\n"; 		// Curlies are not interpreted
  echo "a $arr[0]{1} c\n";	// (Again)
  echo "a $x->y c\n";

	echo <<<END
a $bc d
END;
  echo <<<END
a ${b}c d
END;
	 echo <<<END
a $arr[0] d
END;
    echo <<<END
a $arr[sc] d
END;
    echo <<<END
a $x->y d
END;

	/* 
	 * Complex syntax 
	 */

	echo "a {$b} c\n";
	echo "a {$arr[0]} c\n";
  echo "a {$arr[sc]} c\n"; 		// Note: prints "foo"
	echo "a {$str{0}} c\n"; 		// Curlies _are_ interpreted
	echo "a {$arr[0]{1}} c\n";
	echo "a {$arr["sc"]} c\n";		// while this prints "boo" 
  echo "a {$x->y} c\n";

	echo <<<END
a {$b} c
END;
	echo <<<END
a {$arr[0]} c
END;
	echo <<<END
a {$arr[sc]} c
END;
	echo <<<END
a {$arr{0}} c
END;
	echo <<<END
a {$arr[0]{1}} c
END;
	echo <<<END
a {$arr["sc"]} c
END;
	echo <<<END
a {$x->y} c
END;

	/*
	 * PHP will bark on this, but we don't
	 */
	
#	echo "a $b[ 0 ] c\n";			// We don't generate an error
?>
