@kphp_should_fail k2_skip
/You can't pass callbacks with &references to built-in functions/
<?php

array_map(fn(&$x) => $x+1, [1,2,3]);
