@ok php8
<?php

enum Status {
    case Ok;
    case Fail;
}

var_dump(Status::Ok->name);
var_dump(Status::Fail->name);
