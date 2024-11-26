@ok
<?php

// Define a simple test function for preg_replace
function testPregReplace($pattern, $replacement, $subject, $expected, $description) {
    // Perform the replacement
    $result = preg_replace($pattern, $replacement, $subject);

    // Check if the result matches the expected output
    if ($result === $expected) {
        echo "PASS: $description\n";
    } else {
        echo "FAIL: $description\n";
        echo "Expected: ";
        print_r($expected);
        echo "Got: ";
        print_r($result);
    }
}

// Test case 1: Replace a word with another word in a string
testPregReplace('/\bword\b/', 'replacement', 'This is a word. Another word here.', 'This is a replacement. Another replacement here.', 'Replace word in string');

// Test case 2: Remove digits from a string
testPregReplace('/\d+/', '', 'Phone number: 123-456-7890', 'Phone number: --', 'Remove digits from string');

// Test case 3: Replace multiple spaces with a single space in a string
testPregReplace('/\s+/', ' ', 'This    is   a   test.', 'This is a test.', 'Replace multiple spaces in string');

// Test case 4: Replace words in an array of strings
testPregReplace('/\bword\b/', 'replacement', ['word one', 'another word', 'word again'], ['replacement one', 'another replacement', 'replacement again'], 'Replace word in array');

// Test case 5: Use arrays for pattern and replacement
testPregReplace(['/(\d+)/', '/\bword\b/'], ['number', 'replacement'], '123 word', 'number replacement', 'Array pattern and replacement');

// Test case 6: Use arrays for pattern, replacement, and subject
testPregReplace(['/(\d+)/', '/\bword\b/'], ['number', 'replacement'], ['123 word', 'word 456'], ['number replacement', 'replacement number'], 'Array pattern, replacement, and subject');
