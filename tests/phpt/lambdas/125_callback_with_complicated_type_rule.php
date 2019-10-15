@kphp_should_fail
<?php

var_dump(array_reduce([["a", "b", "c"]], "array_combine", ["1", "2", "3"]));

