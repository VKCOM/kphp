@ok
<?php

function runPregMatchAllTests() {
    // Define test cases
    $testCases = [
        [
            'description' => 'Simple pattern',
            'pattern' => '/foo/',
            'subject' => 'foobar foobar foobar'
        ],
        [
            'description' => 'Pattern with capturing groups',
            'pattern' => '/(\d+)-(\w+)/',
            'subject' => '123-abc 456-def 789-ghi'
        ],
        [
            'description' => 'Named groups',
            'pattern' => '/(?<digits>\d+)?-(?<letters>\w+)?/',
            'subject' => '123-abc -def 789-'
        ],
        [
            'description' => 'UTF-8 string with Unicode modifier',
            'pattern' => '/\p{L}+/u',
            'subject' => 'Hello, 世界 Hello, 世界 Hello, 世界'
        ],
        [
            'description' => 'Dot matches newline with dotall modifier',
            'pattern' => '/foo.*bar/s',
            'subject' => "foo\nbar foo\nbar foo\nbar"
        ],
        [
            'description' => 'Multiline modifier',
            'pattern' => '/^foo/m',
            'subject' => "foo\nbar\nfoo\nbar\nfoo"
        ],
        [
            'description' => 'Case-insensitive modifier',
            'pattern' => '/foo/i',
            'subject' => 'FOO Foo foo'
        ],
        [
            'description' => 'Extended mode with comments',
            'pattern' => '/foo  # Match foo
                         /x',
            'subject' => 'foo bar foo'
        ],
    ];

    foreach ($testCases as $testCase) {
        echo "Test: {$testCase['description']}\n";

        preg_match_all($testCase['pattern'], $testCase['subject'], $matches);
        echo "No flags (PREG_PATTERN_ORDER default):\n";
        var_dump($matches);

        preg_match_all($testCase['pattern'], $testCase['subject'], $matches, PREG_PATTERN_ORDER);
        echo "PREG_PATTERN_ORDER:\n";
        var_dump($matches);

        preg_match_all($testCase['pattern'], $testCase['subject'], $matches, PREG_SET_ORDER);
        echo "PREG_SET_ORDER:\n";
        var_dump($matches);

        preg_match_all($testCase['pattern'], $testCase['subject'], $matches, PREG_OFFSET_CAPTURE);
        echo "PREG_OFFSET_CAPTURE:\n";
        var_dump($matches);

        // preg_match_all($testCase['pattern'], $testCase['subject'], $matches, PREG_UNMATCHED_AS_NULL);
        // echo "PREG_UNMATCHED_AS_NULL:\n";
        // var_dump($matches);

        // preg_match_all($testCase['pattern'], $testCase['subject'], $matches, PREG_OFFSET_CAPTURE | PREG_UNMATCHED_AS_NULL);
        // echo "PREG_OFFSET_CAPTURE, PREG_UNMATCHED_AS_NULL:\n";
        // var_dump($matches);
 
        // preg_match_all($testCase['pattern'], $testCase['subject'], $matches, PREG_OFFSET_CAPTURE | PREG_UNMATCHED_AS_NULL | PREG_SET_ORDER);
        // echo "PREG_OFFSET_CAPTURE, PREG_UNMATCHED_AS_NULL, PREG_SET_ORDER:\n";
	// var_dump($matches);

        // preg_match_all($testCase['pattern'], $testCase['subject'], $matches, PREG_UNMATCHED_AS_NULL | PREG_SET_ORDER);
        // echo "PREG_UNMATCHED_AS_NULL, PREG_SET_ORDER:\n";
        // var_dump($matches);
	
        preg_match_all($testCase['pattern'], $testCase['subject'], $matches, PREG_OFFSET_CAPTURE | PREG_SET_ORDER);
        echo "PREG_OFFSET_CAPTURE, PREG_UNMATCHED_AS_NULL:\n";
	var_dump($matches);
	
	echo "\n";
    }
}

runPregMatchAllTests();
