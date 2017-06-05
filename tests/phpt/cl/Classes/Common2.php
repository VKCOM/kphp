<?php

namespace Classes;

class Common2 {
    const C1 = 'ConstC21';
    const C2 = 'ConstC22';

    public static $f1 = 'FieldC21';
    public static $f2 = 'FieldC22';
    public static $fa1 = array('C2a1', 'C2a2');
    private static $fp1 = 'PrivateC21';

    public static function test1($f1) {
        echo(Common2::C1."\n");
        echo(Common2::C2."\n");
        echo(Common2::$f1."\n");
        echo(Common2::$f2."\n");
        echo(Common2::$fp1."\n");
        echo($f1."\n");
        var_dump(Common2::$fa1);
    }
}
