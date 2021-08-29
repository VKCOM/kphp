@ok php8
<?php

var_dump(INF == "INF");
var_dump(-INF == "-INF");
var_dump(-NAN == "NAN");
var_dump(INF == "1e1000");
var_dump(-INF == "-1e1000");
