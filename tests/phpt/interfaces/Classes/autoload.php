<?php

#ifndef KittenPHP // good
function classAutoLoader($class) {
    $filename = $class.'.php';
    require_once str_replace('\\', '/', $filename);
}

spl_autoload_register('classAutoLoader', true, true);
#endif

