@ok
<?php

function nullableString(): ?string {
    return null;
}

function getString(): string {
    return "";
}

function getInt(): string {
    return "";
}

$int = 1;
$string = "Hello";

echo vsprintf("%%", []);
echo vsprintf("%% %% %%", []);

// simple
echo vsprintf("%d", [$int]);
echo vsprintf("%d", [getInt()]);
echo vsprintf("%s", [$string]);
echo vsprintf("%s", [getString()]);
echo vsprintf("%s %d", [getString(), $int]);
echo vsprintf("%s %s", [getString(), $string]);

// simple with text
echo vsprintf("Hello %d", [$int]);
echo vsprintf("%d world", [getInt()]);
echo vsprintf("%s world", [$string]);
echo vsprintf("Hello %s world %d", [getString(), getInt()]);

// with non-const format
$format = "%s";
echo vsprintf($format, [$int]);
echo vsprintf($format, [$int, getString()]);

// with constant
const FORMAT = "Hello %d%s";
echo vsprintf(FORMAT, [$int, getString()]);
