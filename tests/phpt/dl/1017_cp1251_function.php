@ok
<?php
require_once __DIR__.'/../polyfills.php';

echo cp1251('привет'), "\n";
echo strlen(cp1251('привет')), "\n";

echo cp1251('привет'), "\n";
echo strlen(cp1251('привет')), "\n";

echo cp1251('english only'), "\n";
echo strlen(cp1251('english only')), "\n";

echo cp1251('😑'), "\n";
echo strlen(cp1251('😑')), "\n";
echo vk_win_to_utf8(cp1251('😑')), "\n";
echo vk_win_to_utf8(strlen(cp1251('😑'))), "\n";
