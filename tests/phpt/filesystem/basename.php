@ok
<?php

function test_basename($path, $suffix = '') {
    $result = basename($path, $suffix);
    echo "basename('$path', '$suffix') = '$result'\n";
}

$testCases = [
    ['/usr/local/bin/example.txt', ''],
    ['/usr/local/bin/', ''],
    ['/path/to/directory/', ''],
    ['/usr/local/bin/example.txt', '.txt'],
    ['/path/to/file.txt', '.txt'],
    ['/path/to/file', ''],
    ['/path/to/file.with.multiple.dots.txt', '.txt'],
    ['/just_a_file', ''],
    ['/just_a_file.txt', '.txt'],
    // ['/', ''],
    // ['', ''],
    ['file.with.multiple.dots.txt', '.txt'],
];

foreach ($testCases as $testCase) {
    test_basename($testCase[0], $testCase[1]);
}
