<?php

namespace Classes;

class SelfInsideLambda extends SelfInsideLambdaBase
{
    const X = 10;
    protected static $b = 10;

    public static function run()
    {
        $f = function () {
            return static::get_ten() + self::get_sum(10, 20) + static::id(123) +
                static::$b + parent::X + static::X + self::X;
        };

        $arr = [1, 2, 3];
        uasort($arr, function ($a, $b) {
            return self::get_sum($a, $b);
        });

        return $f();
    }

    public static function get_ten()
    {
        return 10;
    }

    private static function get_sum($a, $b)
    {
        return $a + $b;
    }

    protected static function id($a)
    {
        return $a;
    }
}
