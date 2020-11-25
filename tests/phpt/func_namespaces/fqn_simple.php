@ok
<?php

function f() { return 500; }

var_dump(\count([1]));
var_dump(\f());

var_dump(count([1]));
var_dump(f());
