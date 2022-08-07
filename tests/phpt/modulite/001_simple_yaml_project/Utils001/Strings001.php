<?php
namespace Utils001;

class Strings001 {
    static public function lowercase(string $s): string {
        static $count = 0;  // not global
        $count++;
        \GlobalCl::globalMethod();

        return strtolower($s);
    }
}
