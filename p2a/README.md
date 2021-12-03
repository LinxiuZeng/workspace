This is a basic shell machine with build-in commands: cd, loop, path, and exit. 
And for everything else, create a new process by using fork, wait and execv. 
This machine has two modes: the interactive mode and the batch mode, where the interactive one can 
execute stdin input by user, and the batch mode simply reads all the commands from a standard input file.