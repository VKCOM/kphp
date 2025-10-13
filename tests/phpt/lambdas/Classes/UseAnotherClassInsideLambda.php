<?php

namespace Classes;

use Classes\Constants;

class UseAnotherClassInsideLambda
{
    const U = 10;

    /**
     * @return int
     */
    public function use_another_class_inside_lambda()
    {
        $f = function () {
            return Constants\Numbers::TEN;
        };
        return $f();
    }
}
