@ok
<?php

class Example {
    public $require = "require";
    public $exit = "exit";
}

$obj = new Example();

var_dump("$obj->require {$obj->require}");
var_dump("$obj->exit {$obj->exit}");
