<?php

namespace SakCommon005;

class ErrSak005 {
    const MODE = 1;
    static public int $count = 0;

    static function create() {
        echo "create err";
        return new static;
    }
}
