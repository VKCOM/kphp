@ok
<?php

$text = "asdf 1 sadf";
var_dump(preg_replace_callback("|\d|",function ($matches) { return '99'; } , $text));

