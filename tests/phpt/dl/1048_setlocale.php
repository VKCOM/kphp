@ok
<?php

function setlocale_zero() {
    var_dump("setlocale_zero");

    var_dump("LC_CTYPE 0 -> " . setlocale(LC_CTYPE, "0"));
    var_dump("LC_NUMERIC 0 -> " . setlocale(LC_NUMERIC, "0"));
    var_dump("LC_TIME 0 -> " . setlocale(LC_TIME, "0"));
    var_dump("LC_COLLATE 0 -> " . setlocale(LC_COLLATE, "0"));
    var_dump("LC_MONETARY 0 -> " . setlocale(LC_MONETARY, "0"));
    var_dump("LC_ALL 0 -> " . setlocale(LC_ALL, "0"));
}

function setlocale_return_empty() {
    var_dump("setlocale_empty");

    var_dump("LC_CTYPE \"\" -> " . setlocale(LC_CTYPE, ""));
    var_dump("LC_NUMERIC \"\" -> " . setlocale(LC_NUMERIC, ""));
    var_dump("LC_TIME \"\" -> " . setlocale(LC_TIME, ""));
    var_dump("LC_COLLATE \"\" -> " . setlocale(LC_COLLATE, ""));
    var_dump("LC_MONETARY \"\" -> " . setlocale(LC_MONETARY, ""));
    var_dump("LC_ALL \"\" -> " . setlocale(LC_ALL, ""));

    var_dump("check other (1)");
    setlocale_zero();
}

function setlocale_return_set_ru_RU_CP1251() {
    var_dump("setlocale_set_ru_RU_CP1251");

    var_dump("LC_ALL \"\" -> " . setlocale(LC_ALL, "")); // unset all

    var_dump("LC_CTYPE ru_RU.CP1251 -> " . setlocale(LC_CTYPE, "ru_RU.CP1251"));
    var_dump("LC_CTYPE 0 -> " . setlocale(LC_CTYPE, "0"));

    var_dump("LC_NUMERIC ru_RU.CP1251 -> " . setlocale(LC_NUMERIC, "ru_RU.CP1251"));
    var_dump("LC_NUMERIC 0 -> " . setlocale(LC_NUMERIC, "0"));

    var_dump("LC_TIME ru_RU.CP1251 -> " . setlocale(LC_TIME, "ru_RU.CP1251"));
    var_dump("LC_TIME 0 -> " . setlocale(LC_TIME, "0"));

    var_dump("LC_COLLATE ru_RU.CP1251 -> " . setlocale(LC_COLLATE, "ru_RU.CP1251"));
    var_dump("LC_COLLATE 0 -> " . setlocale(LC_COLLATE, "0"));

    var_dump("LC_MONETARY ru_RU.CP1251 -> " . setlocale(LC_MONETARY, "ru_RU.CP1251"));
    var_dump("LC_MONETARY 0 -> " . setlocale(LC_MONETARY, "0"));

    var_dump("LC_ALL 0 -> " . setlocale(LC_ALL, "0"));

    var_dump("LC_ALL \"\" -> " . setlocale(LC_ALL, "")); // unset all

    var_dump("check other (2)");
    setlocale_zero();

    var_dump("LC_ALL ru_RU.CP1251 -> " . setlocale(LC_ALL, "ru_RU.CP1251"));

    var_dump("check other (3)");
    setlocale_zero();

    var_dump("LC_ALL \"\" -> " . setlocale(LC_ALL, "")); // unset all
}

function setlocale_return_correct_LC_ALL_changes() {
    var_dump("setlocale_correct_LC_ALL_changes");

    var_dump("LC_ALL en_US.UTF-8 -> " . setlocale(LC_ALL, "en_US.UTF-8"));
    var_dump("LC_ALL 0 -> " . setlocale(LC_ALL, "0"));

    var_dump("LC_COLLATE ru_RU.CP1251 -> " . setlocale(LC_COLLATE, "ru_RU.CP1251"));
    var_dump("LC_ALL 0 -> " . setlocale(LC_ALL, "0"));

    var_dump("LC_COLLATE en_US.UTF-8 -> " . setlocale(LC_COLLATE, "en_US.UTF-8"));
    var_dump("LC_ALL 0 -> " . setlocale(LC_ALL, "0"));
}


function setlocale_numeric_cp1251() {
    function printf_numeric() {
        printf("%.2f\n", 1234.56);
    }

    var_dump("setlocale_numeric_cp1251");
    setlocale(LC_NUMERIC, 'ru_RU.CP1251');
    printf_numeric();
    setlocale(LC_NUMERIC, 'C');
    printf_numeric();
}


setlocale_return_empty();
setlocale_return_set_ru_RU_CP1251();
setlocale_return_correct_LC_ALL_changes();
setlocale_numeric_cp1251();
