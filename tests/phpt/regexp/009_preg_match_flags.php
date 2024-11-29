@ok
<?php

// PREG_UNMATCHED_AS_NULL tests are commented out since KPHP doesn't support that flag

function runPregMatchTests() {
    // Test 1: Simple match with no flags
    $pattern = '/foo/';
    $subject = 'foobar';
    preg_match($pattern, $subject, $matches);
    echo "Test 1: ";
    var_dump($matches);

    // Test 2: Case-insensitive match
    $pattern = '/foo/i';
    $subject = 'FOObar';
    preg_match($pattern, $subject, $matches);
    echo "Test 2: ";
    var_dump($matches);

    // Test 3: Named groups
    $pattern = '/(?<first>foo)(?<second>bar)/';
    $subject = 'foobar';
    preg_match($pattern, $subject, $matches);
    echo "Test 3: ";
    var_dump($matches);

    // Test 4: UTF-8 string with Unicode flag
    $pattern = '/\p{L}+/u'; // Matches one or more Unicode letters
    $subject = 'Hello, 世界';
    preg_match($pattern, $subject, $matches);
    echo "Test 4: ";
    var_dump($matches);

    // Test 5: Empty pattern
    $pattern = '//';
    $subject = 'foobar';
    preg_match($pattern, $subject, $matches);
    echo "Test 5: ";
    var_dump($matches);

    // Test 6: Empty subject
    $pattern = '/foo/';
    $subject = '';
    preg_match($pattern, $subject, $matches);
    echo "Test 6: ";
    var_dump($matches);

    // Test 7: Pattern with optional groups and PREG_UNMATCHED_AS_NULL
    // $pattern = '/(foo)?(bar)?/';
    // $subject = 'foo';
    // preg_match($pattern, $subject, $matches, PREG_UNMATCHED_AS_NULL);
    // echo "Test 7: ";
    // var_dump($matches);

    // Test 8: Multiline flag
    $pattern = '/^foo/m';
    $subject = "foo\nbar\nfoo";
    preg_match($pattern, $subject, $matches);
    echo "Test 8: ";
    var_dump($matches);

    // Test 9: Dot matches newline (dotall flag)
    $pattern = '/foo.*bar/s';
    $subject = "foo\nbar";
    preg_match($pattern, $subject, $matches);
    echo "Test 9: ";
    var_dump($matches);

    // Test 10: Anchors and boundaries
    $pattern = '/^foo/';
    $subject = 'foobar';
    preg_match($pattern, $subject, $matches);
    echo "Test 10: ";
    var_dump($matches);

    // Test 11: PREG_OFFSET_CAPTURE flag
    $pattern = '/bar/';
    $subject = 'foobar';
    preg_match($pattern, $subject, $matches, PREG_OFFSET_CAPTURE);
    echo "Test 11: ";
    var_dump($matches);

    // Test 12: Unmatched patterns at the end
    $pattern = '/(foo)?(bar)?(baz)?/';
    $subject = 'foo';
    preg_match($pattern, $subject, $matches);
    echo "Test 12: ";
    var_dump($matches);

    // Test 13: Unmatched patterns at the end with PREG_UNMATCHED_AS_NULL
    // preg_match($pattern, $subject, $matches, PREG_UNMATCHED_AS_NULL);
    // echo "Test 13: ";
    // var_dump($matches);
}

runPregMatchTests();
