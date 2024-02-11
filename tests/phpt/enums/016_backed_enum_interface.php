@ok php8
<?php
enum Status : int {
    case Ok = 1;
    case Fail = 0;
}

echo Status::Ok instanceof \BackedEnum;
echo Status::Ok instanceof \UnitEnum;
echo Status::Fail instanceof \BackedEnum;
echo Status::Fail instanceof \UnitEnum;

enum A {
    case B;
}

echo A::B instanceof \BackedEnum;
