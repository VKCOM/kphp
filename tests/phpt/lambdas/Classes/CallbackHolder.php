<?php

namespace Classes;

class CallbackHolder
{
    public function get_callback(): callable
    {
        return function ($a, $b) {
            return $a + $b;
        };
    }
}
