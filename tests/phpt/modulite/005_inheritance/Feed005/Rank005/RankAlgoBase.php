<?php

namespace Feed005\Rank005;

abstract class RankAlgoBase {
    use WithCopy;

    static function rank() {
        $impl_str = static::getImplStr();
        echo $impl_str, "\n";
    }

    abstract static function getImplStr(): string;
}
