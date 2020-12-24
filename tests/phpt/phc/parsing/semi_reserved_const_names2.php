@ok
<?php

class ExampleConsts {
    const TRUE = 'TRUE';
    const FALSE = 'FALSE';
    const NULL = 'NULL';
    const NAN = 'NAN';
    const __LINE__ = '__LINE__';
    const __FILE__ = '__FILE__';
}

var_dump(ExampleConsts::TRUE);
var_dump(ExampleConsts::FALSE);
var_dump(ExampleConsts::NULL);
var_dump(ExampleConsts::NAN);
var_dump(ExampleConsts::__LINE__);
var_dump(ExampleConsts::__FILE__);
