<?php

namespace Feed126;

trait PrivTrait {
    function showThisClass() {
        echo get_class($this), "\n";
    }
}
