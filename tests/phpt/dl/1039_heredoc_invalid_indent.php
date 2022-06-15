@kphp_should_fail
/Invalid body indentation level \(expecting an indentation level of at least 1\)/
/Invalid body indentation level \(expecting an indentation level of at least 2\)/
/Invalid body indentation level \(expecting an indentation level of at least 3\)/
/Invalid body indentation level \(expecting an indentation level of at least 4\)/
/Invalid indentation \- tabs and spaces cannot be mixed/
/Invalid indentation \- tabs and spaces cannot be mixed/
/Invalid indentation \- tabs and spaces cannot be mixed/
/Invalid indentation \- tabs and spaces cannot be mixed/
/Invalid indentation \- tabs and spaces cannot be mixed/
/Invalid indentation \- tabs and spaces cannot be mixed/
/Invalid indentation \- tabs and spaces cannot be mixed/
<?php

echo <<<END
a
 END;

echo <<<END
 a
  END;

echo <<<END
  b
   a
   END;

echo <<<END
    b
   a
    END;

// mix tabs and spaces below
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
		END;

echo <<<END
 	a
  END;

echo <<<END
	 a
  END;

echo <<<END
		a
  END;

