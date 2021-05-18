@ok
<?php

use function strlen;
use function count;

var_dump(strlen('abc'));
var_dump(\strlen('abc'));
var_dump(count([1]));
var_dump(\count([1]));
