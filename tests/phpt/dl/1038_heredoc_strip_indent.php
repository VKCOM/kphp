@ok
<?php

function test_strip_indent_space() {
echo <<<END
  a
END;

echo <<<END
  a
 END;

echo <<<END
  a
  END;

echo <<<END
  a
 b
END;

echo <<<END
  a
 b
 END;

echo <<<END
 a
 b
 END;

echo <<<END
  a
  b
  END;

echo <<<END
  	a
  		b
  END;

echo <<<END
      a
    b
   c
   END;
}

function test_strip_indent_tabs() {
echo <<<END
		a
END;

echo <<<END
		a
	END;

echo <<<END
		a
		END;

echo <<<END
		a
	b
END;

echo <<<END
		a
	b
	END;

echo <<<END
	a
	b
	END;

echo <<<END
		a
		b
		END;

echo <<<END
		 a
		  b
		END;

echo <<<END
						a
				b
			c
			END;
}

function test_strip_indent_nowdoc() {
echo <<<'END'
  a
END;

echo <<<'END'
  a
 END;

echo <<<'END'
  a
  END;

echo <<<'END'
  a
 b
END;

echo <<<'END'
  a
 b
 END;

echo <<<'END'
 a
 b
 END;

echo <<<'END'
  a
  b
  END;

echo <<<'END'
  	a
  		b
  END;

echo <<<'END'
      a
    b
   c
   END;
}

test_strip_indent_space();
test_strip_indent_tabs();
test_strip_indent_nowdoc();
