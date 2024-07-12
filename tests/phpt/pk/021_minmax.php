@ok k2_skip
<?php

var_dump(min([1]));
var_dump(min([1, 2, 3]));
var_dump(min(...[1, 2, 3], ...[3,4, 5]));
var_dump(min(1, 2, 3));
var_dump(max('611395.82', '758.86').'<br>'.max('b', 'a', 'd', 'c'));
