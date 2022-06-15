@ok
<?php

function strip_indent_space() {
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

function strip_indent_tabs() {
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

strip_indent_space();
strip_indent_tabs();
