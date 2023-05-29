@ok php8
<?php

enum MyBool {
    case T;
    case F;
}

$v = MyBool::T;
echo $v->name;

const c = MyBool::F;
echo c->name;

echo MyBool::F->name;
echo MyBool::T->name;
