#Accepts the following arguments: the first argument is a full path to a file (including filename) on the filesystem, referred to below as writefile; t
# the second argument is a text string which will be written within this file, referred to below as writestr
#
#Exits with value 1 error and print statements if any of the arguments above were not specified
#
#Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist. 
#Exits with value 1 and error print statement if the file could not be created.

if [ $# -ne 2 ]; then
    echo "Error: Two arguments are required. Please provide the path to a file and the string to be written."
    exit 1
fi

writefile=$1
writestr=$2

echo "writefile: $writefile"
echo "writestr: $writestr"

mkdir -p "$(dirname "$writefile")" || {
    echo "Error: Failed to create directory."
    exit 1
}

echo "$writestr" > "$writefile" || {
    echo "Error: Failed to write to file."
    exit 1
}
