@kphp_should_fail
/Invalid body indentation level \(expecting an indentation level of at least 1\)/
/Invalid body indentation level \(expecting an indentation level of at least 2\)/
/Invalid body indentation level \(expecting an indentation level of at least 3\)/
/Invalid body indentation level \(expecting an indentation level of at least 4\)/
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

