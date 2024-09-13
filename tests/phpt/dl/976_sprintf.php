@ok k2_skip
<?php

printf ("-<a>-%d-<b>-%s-<c>-%f\n", 1, "123", 0.2);
echo sprintf ("-<a>-%d-<b>-%s-<c>-%f\n", 1, "123", 0.2);
echo sprintf('экранирование процентов %s%%\n', 5);
echo sprintf("экранирование процентов %%\n");

vprintf ("-<a>-%d-<b>-%s-<c>-%f\n", array (1, "123", 0.2));
echo vsprintf ("-<a>-%d-<b>-%s-<c>-%f\n", array (1, "123", 0.2));
echo vsprintf('экранирование процентов %s%%\n', array (5));
echo vsprintf("экранирование процентов %%\n", array());
