<?php

namespace Feed005\Rank005;

trait PubTrait {
    public int $i = 0;
    static public int $count = 0;

    static function pubStaticFn() {
        self::$count++;
        echo "pubStaticFn in trait ", self::$count, "\n";
    }

    function pubInstanceFn() {
        echo "pubInstanceFn in trait ", get_class($this), "\n";
    }
}
