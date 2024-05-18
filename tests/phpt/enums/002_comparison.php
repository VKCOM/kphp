@ok php8
<?php

enum MyBool {
    case T;
    case F;

    const SecT = self::T;
    const SecF = self::F;
}

$t_var = MyBool::T;
const t_const = MyBool::T;

var_dump($t_var === MyBool::T);
var_dump(t_const === MyBool::T);
var_dump(MyBool::SecT === MyBool::T);

$f_var = MyBool::F;
const f_const = MyBool::F;

var_dump($f_var === MyBool::F);
var_dump(f_const === MyBool::F);
var_dump(MyBool::SecF === MyBool::F);
