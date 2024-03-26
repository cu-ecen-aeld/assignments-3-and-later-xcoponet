/* 
* Accepts the following arguments: the first argument is a full path to a file (including filename) on the filesystem, referred to below as writefile;
 the second argument is a text string which will be written within this file, referred to below as writestr

* Exits with value 1 error and print statements if any of the arguments above were not specified

* Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist. 
Exits with value 1 and error print statement if the file could not be created.

* Setup syslog logging for your utility using the LOG_USER facility.

* Use the syslog capability to write a message “Writing <string> to <file>” where <string> is the text string written to file (second argument) and <file> is the file created by the script.  This should be written with LOG_DEBUG level.

* Use the syslog capability to log any unexpected errors with LOG_ERR level.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    // Check if the required arguments are specified
    if (argc < 3) {
        fprintf(stderr, "Error: Insufficient arguments\n");
        syslog(LOG_ERR, "Error: Insufficient arguments");
        exit(1);
    }

    // Extract the arguments
    char *writefile = argv[1];
    char *writestr = argv[2];

    // Create the file
    FILE *file = fopen(writefile, "w");
    if (file == NULL) {
        syslog(LOG_ERR, "Error creating file: %s", writefile);
        exit(1);
    }

    // Write the string to the file
    fprintf(file, "%s", writestr);
    fclose(file);

    // Setup syslog logging
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    // Log the message
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

    // Clean up syslog
    closelog();

    return 0;
}
