<?php

namespace Classes;

class CallbackHolder
{
    public function get_callback()
    {
        return function ($a, $b) {
            return $a + $b;
        };
    }
}