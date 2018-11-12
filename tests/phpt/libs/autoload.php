<?php

#ifndef KittenPHP // good

function ClassAutoLoader($class) {
    require_once str_replace('\\', '/', $class.'.php');
}

spl_autoload_register('ClassAutoLoader', true, true);

#endif
