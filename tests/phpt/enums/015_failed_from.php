@ok php8
<?php

enum Status : int {
    case Ok = 200;
}

try {
    $v = Status::from(0);
    echo "unreachable\n";
} catch (ValueError $e) {
    echo $e->getCode() . "\n";
}
