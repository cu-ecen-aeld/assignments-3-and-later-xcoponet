
#!/bin/bash

# Load the driver
./aesdchar_load

# Test the driver
echo "Testing the driver" > /dev/aesdchar
echo "hello" > /dev/aesdchar
echo "there" > /dev/aesdchar
echo "john" > /dev/aesdchar
echo "doe speaking" > /dev/aesdchar
echo "6" > /dev/aesdchar
echo "7" > /dev/aesdchar
echo "8" > /dev/aesdchar
echo "9" > /dev/aesdchar
echo "10" > /dev/aesdchar
echo "11" > /dev/aesdchar
cat /dev/aesdchar

# Unload the driver
./aesdchar_unload
