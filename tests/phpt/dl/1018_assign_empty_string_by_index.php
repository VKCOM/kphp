@ok
<?php
// dl/435_base64_encode.php
// php7 не разрешает присваивать в строку по индексу другую пустую строку, раньше это было как присваивание '\0', 
// либо удаление символа, если он в середине. Теперь это warning и ничего не делает

/**
 * @param mixed $s
 */
function run($s) {
    $s[3] = '';
    var_dump($s);
    $s[1] = '';
    var_dump($s);
    $s[10] = '';
    var_dump($s);

    $s[10] = 'a';
    var_dump($s);

    $s[1] = 'Hello, World!';
    var_dump($s);
}

run('123');
