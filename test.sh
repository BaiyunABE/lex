#!/bin/bash

# Check if lex executable exists
if [ ! -f "./lex" ]; then
  echo "Error: lex executable not found in current directory"
  exit 1
fi

# Check if test directory exists
if [ ! -d "./test" ]; then
  echo "Error: test directory not found"
  exit 1
fi

echo "Starting lexer performance tests..."
echo "========================================"

# Loop through test files
for i in {1..25}; do
  test_file="test/t${i}.c"

  # Check if test file exists
  if [ ! -f "$test_file" ]; then
    echo "Warning: $test_file not found, skipping..."
    continue
  fi

  echo "Testing $test_file:"
  wc -l "$test_file"
  time ./lex "$test_file" >/dev/null
  echo "----------------------------------------"
done

test_file="lex.c"

# Check if test file exists
if [ ! -f "$test_file" ]; then
  echo "Warning: $test_file not found, skipping..."
  continue
fi

echo "Testing $test_file:"
wc -l "$test_file"
time ./lex "$test_file" >/dev/null
echo "----------------------------------------"

echo "All tests completed!"
