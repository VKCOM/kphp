@ok
<?php
#ifndef KittenPHP
require_once __DIR__.'/../dl/polyfill/fork-php-polyfill.php';
require_once 'Classes/autoload.php';

function instance_to_array($instance) { 
    if (!is_object($instance)) {
        return $instance;
    }

    $convert_array_of_instances = function (&$a) use (&$convert_array_of_instances) {
        foreach ($a as &$v) {
            if (is_object($v)) {
                $v = instance_to_array($v);
            } elseif (is_array($v)) {
                $convert_array_of_instances($v);
            }
        }
    };

    $a = (array) $instance;
    $convert_array_of_instances($a);

    return $a;
}

function tuple(...$args) { return $args; }
#endif

class Temp {
    public $x;
}

function demo() {
    sched_yield();
    $t = new Temp();
    $t->x = tuple(123, "asdf", new \Classes\A());

    return $t;
}

$ii = fork(demo());
$t = wait_result($ii);
echo "done\n";

var_dump(instance_to_array($t));
