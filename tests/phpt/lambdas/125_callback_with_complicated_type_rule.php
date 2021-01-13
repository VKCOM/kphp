@ok
<?php

var_dump(array_reduce([["a", "b", "c"]], "array_combine", ["1", "2", "3"]));
var_dump(array_reduce([["a", "b", "c"]], "array_combine", [1, 2, 3]));
var_dump(array_reduce([[1, 2, 3]], "array_combine", [1, 2, 3]));

