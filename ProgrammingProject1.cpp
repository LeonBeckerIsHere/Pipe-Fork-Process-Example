//File: ProgrammingProject1.cpp
//Author: Leon Becker
//NetID: lab160730

/*
OBJECTIVE
*********
Design a program using ordinary pipes in which one process sends a string message to a second
process, and the second process reverses the case of each character in the message and sends it back
to the first process. 

The program should take a file name <name> as input and use it as follows:
• <name>-1.txt: the name of the file that contains the message to be transformed.
• <name>-2.txt: the name of the file that contains the message received by the second process
from the first process.
• <name>-3.txt: the name of the file that contains the transformed message sent by the second
process to the first process.
• <name>-4.txt: the name of the file that contains the transformed message received by the first
process from the second process.
You should write your program in C/C++ and it should run on a UNIX/Linux system. You
will need to use fork( ) and pipe( ) system calls in this program. You can learn more about these
and other related system calls in the man pages.
*/

#include <sys/types.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <stdio.h>
#include <string>
#include <unistd.h>

//Global constants to help with readability
const static int BUFFER_SIZE = 1024;
const static int READ_END = 0;
const static int WRITE_END = 1;
const static int PARENT_TO_CHILD = 0;
const static int CHILD_TO_PARENT = 1;

//@brief checks if the proper amount of command line arguments have been provided
//@param argc   argument count
void checkForCommandLineArgument(int argc){
    if(argc != 2){
        fprintf(stderr, "Program does not meet argument requirements");
        exit(1);
    }
}

//@brief checks if the file opened correctly
//@param file   open file stream
void isFileOpen(const std::fstream &file){
    if(!file.is_open()){
        fprintf(stderr, "File error");
        exit(1);
    }
}

//@brief empties out the contents of the output files to prevent appends from multiple runs
//@param fileName   prefix file name to the -2,-3,-4 .txt files
void resetFiles(const std::string &fileName){
    std::fstream file;
    for(int i = 2; i < 5; i++){
        file.open(fileName + "-" + std::to_string(i) + ".txt", std::fstream::out | std::fstream::trunc);
        isFileOpen(file);
        file.close();
    }
}

//@brief writes data to a file
//@param fileName   name of the file
//@param data       data to be written to the file
void writeToFile(const std::string &fileName, const std::string &data){
    std::fstream outFile;
    outFile.open(fileName, std::fstream::out | std::fstream::app);
    isFileOpen(outFile);
    outFile << data << std::endl;
    outFile.flush();
    outFile.close();
    
}

//@brief create a pipe
//@param fd     file descriptor to create the pipe
void createPipe(int fd[2]){
    if(pipe(fd) == -1){
        fprintf(stderr, "Pipe failed");
        exit(1);
    }
}

//@brief check if the fork failed or not
//@param pid    process id 
void checkForkExecuted(const pid_t &pid){
    if(pid < 0){
        fprintf(stderr, "Fork failed:");
        exit(1);
    }
}

//@brief write to a provided pipe
//@param pipe_fd    pipe to write to
//@param msg        message to write to pipe
//@param msg_len    the length of the message
void writeToPipe(int pipe_fd[2], char* msg, int msg_len){
    close(pipe_fd[READ_END]);
    write(pipe_fd[WRITE_END], msg, msg_len);
    close(pipe_fd[WRITE_END]);
}

//@brief read from a provided pipe and return the data in a std::string
//@param pipe_fd   pipe to read from
std::string readFromPipe(int pipe_fd[2]){
    char msg[BUFFER_SIZE];
    close(pipe_fd[WRITE_END]);
    read(pipe_fd[READ_END], msg, BUFFER_SIZE);
    close(pipe_fd[READ_END]);
    
    std::string returnString(msg);
    return returnString;
}

//@brief perform parent process operations
//@param pid        process id
//@param pipe_fd    contains two pipes one that is CHILD_TO_PARENT and one that is PARENT_TO_CHILD
//@param line       the data to write to the PARENT_TO_CHILD pipe
//@param fileName   prefix to file name to be written to
void parentProcess(const pid_t &pid, int pipe_fd[2][2], std::string line, const std::string &fileName){
    if(pid > 0){
        writeToPipe(pipe_fd[PARENT_TO_CHILD], const_cast<char *>(line.c_str()), line.length()+1);
        std::string msg = readFromPipe(pipe_fd[CHILD_TO_PARENT]);
        writeToFile(fileName + "-4.txt", msg);
    }
}

//@brief changes the case for an alphabetic character
//@param c      character to change the case of
char changeCase(char c){
    if(!std::isalpha(c))
        return c;

    if(std::isupper(c))
        return std::tolower(c);
    else
        return std::toupper(c);
}

//@brief reverse the case for a given string
//@param s      string to reverse the case of 
void reverseCase(std::string &s){
    std::transform(s.begin(),s.end(),s.begin(),changeCase);
}

//@brief perform child process operations
//@param pid        process id
//@param pipe_fd    contains two pipes one that is CHILD_TO_PARENT and one that is PARENT_TO_CHILD
//@param fileName   prefix for fileName for writing to files
void childProcess(const pid_t &pid, int pipe_fd[2][2], const std::string &fileName){
    if(pid == 0){
        std::string msg = readFromPipe(pipe_fd[PARENT_TO_CHILD]);
        writeToFile(fileName + "-2.txt", msg);
        reverseCase(msg);
        writeToFile(fileName + "-3.txt", msg);
        writeToPipe(pipe_fd[CHILD_TO_PARENT], const_cast<char *>(msg.c_str()), msg.length()+1);
        exit(0);
    }
}

//@breif main program that accomplishes the objectives given for ProgrammingProject1
int main(int argc, char** argv){
    checkForCommandLineArgument(argc);

    std::string line;
    
    //Convert C-String to std::string
    std::string fileName(argv[1]);

    //Open fileName-1.txt
    std::fstream inputFile;
    inputFile.open(fileName + "-1.txt", std::fstream::in);

    isFileOpen(inputFile);
    
	resetFiles(fileName);
	
    //For each line in the input file create two pipes, fork child process, and comsume the data
    while(std::getline(inputFile, line)){
        int pipe_fd[2][2];
        createPipe(pipe_fd[PARENT_TO_CHILD]);
        createPipe(pipe_fd[CHILD_TO_PARENT]);
                
        pid_t pid = fork();
        checkForkExecuted(pid);
        parentProcess(pid, pipe_fd, line, fileName);
        childProcess(pid, pipe_fd, fileName);
        

    }
    inputFile.close();

    return 0;
}
