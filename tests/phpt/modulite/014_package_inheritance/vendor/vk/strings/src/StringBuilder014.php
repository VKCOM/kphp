<?php

namespace VK\Strings;

class StringBuilder014 {
    static public $inited = false;

    private string $s = '';

    public function __construct(string $s) {
        $this->s = $s;
    }

    public function append(string $s) {
        $this->s = Utils014\Append014::concatStr($this->s, $s);
    }

    public function get(): string {
        return $this->s;
    }
}
