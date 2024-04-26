
#!/bin/bash

# Load the driver
./aesdchar_load

# Test the driver
echo "Testing the driver" > /dev/aesdchar
cat /dev/aesdchar

# Unload the driver
./aesdchar_unload
