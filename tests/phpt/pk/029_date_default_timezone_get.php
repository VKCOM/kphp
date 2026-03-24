@ok
<?php

date_default_timezone_set('Europe/Moscow');
var_dump(date_default_timezone_get());
date_default_timezone_set('Etc/GMT-3');
var_dump(date_default_timezone_get());
