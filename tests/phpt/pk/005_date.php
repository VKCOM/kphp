@ok k2_skip
<?php
$all = '\d:d \D:D \j:j \l:l \N:N \S:S \w:w \z:z \W:W \F:F \m:m \M:M \n:n \t:t \L:L \o:o \Y:Y \y:y \a:a \A:A \B:B \g:g \G:G \h:h \H:H \i:i \s:s \u:u \e:e \I:I \O:O \P:P \T:T \Z:Z \c:c \r:r \U:U';

$start = 1425997838;

date_default_timezone_set("Europe/Moscow");
for ($i = 0; $i <= 6000; $i++) {
    print date($all, $start + $i * (60 * 60 * 24 - 53)) . "\n";
}
for ($i = 0; $i <= 6000; $i++) {
    print gmdate($all, $start + $i * (60 * 60 * 24 - 53)) . "\n";
}



