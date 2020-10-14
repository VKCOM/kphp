<?php

namespace Classes;

class Common {
    const C1 = 'Const1';
    const C2 = 'Const2';

    public static $f1 = 'Field1';
    public static $f2 = 'Field2';
    public static $fa1 = array('a1', 'a2');
    private static $fp1 = 'Private1';

    /**
     * @param string $f1
     */
    public static function test1($f1) {
        echo(Common::C1."\n");
        echo(Common::C2."\n");
        echo(Common::$f1."\n");
        echo(Common::$f2."\n");
        echo(Common::$fp1."\n");
        echo($f1."\n");
        var_dump(Common::$fa1);
    }

    /**
     * @param string $f1
     */
    public static function test2($f1) {
        Common2::test1($f1);
    }
}
