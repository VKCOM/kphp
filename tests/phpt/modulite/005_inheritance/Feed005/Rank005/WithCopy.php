<?php

namespace Feed005\Rank005;

trait WithCopy {
    static function copyMe() {
        echo "copy from trait", "\n";
    }
}
