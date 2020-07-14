<?php

namespace Classes;

class PassCallbackInsideClass
{
    /**
     * @kphp-template $callback
     */
    public function call_callback($callback)
    {
        return $callback(10, 20, 30);
    }

    /**
     * @kphp-infer
     * @return int
     */
    public function use_callback()
    {
        return $this->call_callback(function ($a, $b, $c) {
            return $a + $b + $c;
        });
    }
}
