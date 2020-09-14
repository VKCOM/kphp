@ok
<?php

require_once 'kphp_tester_include.php';

/** @var Classes\IDo[] $tasks */
$tasks = [new Classes\A(), new Classes\B()];
foreach ($tasks as $task) {
   $task->do_it(10, 20);
}

