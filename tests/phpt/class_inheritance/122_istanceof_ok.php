@ok
<?php

interface IBase {}
class Base implements IBase {}
class ExtBase extends Base {}

function f(IBase $b) {
    if ($b instanceof ExtBase) {
        echo 1;
    } else if ($b instanceof Base) {
        echo 2;
    }
}

f(new Base);
f(new ExtBase);
