<?php

#ifndef KPHP // good

function LibClassAutoLoader($class) {
    require_once str_replace('\\', '/', $class.'.php');
}

spl_autoload_register('LibClassAutoLoader', true, true);

#endif
