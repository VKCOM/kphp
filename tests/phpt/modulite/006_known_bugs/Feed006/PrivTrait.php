<?php

namespace Feed006;

trait PrivTrait {
    function showThisClass() {
        echo get_class($this), "\n";
    }
}
