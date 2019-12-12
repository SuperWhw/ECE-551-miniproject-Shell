#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "myShell.h"

extern char ** environ;  //manpage environ: The variable environ points to an array of pointers to strings called the "environment"

/*Reference:
1.man fork; man execve; man waitpid; man chdir; man setenv; man environ
2.http://www.cplusplus.com/reference/string/string/find/
3.https://linux.die.net/man/3/getcwd
4.ECE551 assignment 098_dedup
5.http://www.cplusplus.com/reference/string/string/replace/
6.http://www.cplusplus.com/reference/string/string/erase/
7.https://www.gnu.org/software/libc/manual/html_node/index.html#toc-Introduction-1
*/

void runShell() {
  Shell myshell(environ);  //initialize myshell with environment variables
  std::string line = "";
  std::cout << "ffosh:" << myshell.getCurrPath() << " $";
  while (std::getline(std::cin, line)) {
    try {
      if (line == "exit") {
        break;
      }
      myshell.initPath();
      std::vector<std::string> argvString = myshell.splitLine(line);
      if (argvString.size() == 0) {  //skip blank line
        std::cout << "ffosh:" << myshell.getCurrPath() << " $";
        continue;
      }

      for (size_t i = 0; i < argvString.size(); ++i) {
        argvString[i] = myshell.parseVar(argvString[i]);  //replace $VAR with value
      }

      if (argvString[0] == "cd") {
        myshell.myCd(argvString);
      }
      else if (argvString[0] == "set") {
        myshell.mySet(argvString, line);
      }
      else if (argvString[0] == "export") {
        myshell.myExport(argvString);
      }
      else if (argvString[0] == "rev") {
        myshell.myRev(argvString);
      }
      else {
        pid_t cpid = fork();
        if (cpid == -1) {  //Fork failed
          std::cerr << "Fork failed!" << std::endl;
          exit(EXIT_FAILURE);
        }
        else if (cpid == 0) {  //Child process

          myshell.childProcess(argvString);
        }
        else if (cpid > 0) {  //Parent process
          myshell.parentProcess(cpid);
        }
      }

      std::cout << "ffosh:" << myshell.getCurrPath() << " $";
    }
    catch (std::exception & e) {  //catch all exceptions
      std::cerr << e.what() << std::endl;
      continue;
    }
  }
}

int main(void) {
  runShell();
  return EXIT_SUCCESS;
}
