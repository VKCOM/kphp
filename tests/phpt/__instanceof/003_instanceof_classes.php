@ok
<?php

class Base {}
class ExtBase extends Base {}
class Ext2Base extends Base {}

function f(Base $b) {
    if ($b instanceof ExtBase) {
        echo 1;
    } else if ($b instanceof Ext2Base) {
        echo 2;
    }
}

f(new ExtBase);
f(new Ext2Base);
