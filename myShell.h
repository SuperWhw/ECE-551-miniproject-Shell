#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

class Shell
{
 private:
  std::vector<char *> argv;
  std::string ECE551PATH;
  std::vector<std::string> myPATH;
  char ** envp;
  std::map<std::string, std::string> localVar;

 public:
  Shell(char ** environ) : envp(environ) {
    ECE551PATH = getPath();               //initialize ECE551PATH
    localVar["ECE551PATH"] = ECE551PATH;  //put ECE551PATH into localVar
  }

  /*This part deals with string parsing*/
  std::vector<std::string> splitPath();
  std::string parseVar(std::string);
  std::vector<std::string> splitLine(std::string);
  /*----------------end----------------*/

  /*This part deals with special command*/
  void myCd(std::vector<std::string>);
  void mySet(std::vector<std::string>, std::string);
  void myExport(std::vector<std::string>);
  void myRev(std::vector<std::string>);
  /*----------------end-----------------*/

  std::string getPath() {               //get PATH from environment
    std::string path = getenv("PATH");  //string is more convenient than char *
    if (path == "") {
      std::cerr << "PATH is empty!" << std::endl;
      exit(EXIT_FAILURE);
    }
    return path;
  }

  std::string getCurrPath() {  //get current path
    char buff[100];
    getcwd(buff, 100);
    std::string buff_str = buff;
    return buff_str;
  }

  void initPath() {                       //initialize ECE551PATH
    ECE551PATH = localVar["ECE551PATH"];  //ECE551PATH
    std::vector<std::string> v{"export", "ECE551PATH"};
    myExport(v);
    myPATH = splitPath();  //initialize myPATH
  }

  /*Child Process*/
  void childProcess(std::vector<std::string> argvString) {
    //char * envp[] = {NULL};
    argvString[0] = findArgv0(argvString[0]);  //find proper argv[0]

    for (size_t i = 0; i < argvString.size(); ++i) {  //convert string to char*
      argv.push_back(const_cast<char *>(argvString[i].c_str()));
    }
    argv.push_back(NULL);
    /*just for test
    for(size_t i = 0; i < argv.size(); ++i) {
      std::cout << "in child process argv: " << argv[i] << "\n";
    }
    
    for(auto i = localVar.begin(); i != localVar.end(); ++i) {
      std::cout << i->first << " " << i->second << "\n";
    }
    */
    execve(argv[0], &argv[0], envp);
    std::cerr << "execve failed!" << std::endl;
    exit(EXIT_FAILURE);  //NEED EXIT!!!
  }

  /*Parent Process*/
  void parentProcess(pid_t cpid) {
    pid_t w;
    int wstatus;
    do {
      w = waitpid(cpid, &wstatus, WUNTRACED | WCONTINUED);
      if (w == -1) {
        std::cerr << "waitpid" << std::endl;
        exit(EXIT_FAILURE);
      }
      if (WIFEXITED(wstatus)) {
        if (WEXITSTATUS(wstatus) == 0) {
          std::cout << "Program was successful" << std::endl;
        }
        else {
          std::cout << "Program failed with code " << WEXITSTATUS(wstatus) << std::endl;
        }
      }
      else if (WIFSIGNALED(wstatus)) {
        std::cout << "Terminated by signal " << WTERMSIG(wstatus) << std::endl;
      }
    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
  }

  std::string findArgv0(std::string argvString) {     //find proper argv[0]
    if (argvString.find('/') == std::string::npos) {  //not have '/' in command name
      char * arg = const_cast<char *>(argvString.c_str());
      bool find = false;
      for (size_t i = 0; i < myPATH.size(); ++i) {  //find command in myPATH
        find = findCommand(const_cast<char *>(myPATH[i].c_str()), arg);
        if (find == true) {
          return (myPATH[i] + '/' + argvString);
        }
      }
      if (find == false) {  //command not found
        std::cerr << "Command " << argvString << " not found" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    else {  //have '/' in command line
      struct stat sb;
      if (lstat(argvString.c_str(), &sb) == -1) {  //command not found
        std::cerr << "Command " << argvString << " not found" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
    return argvString;
  }

  bool findCommand(char * path, char * arg) {  //mainly from 098_dedup
    struct stat sb;
    if (lstat(path, &sb) == -1) {
      perror("lstat");
      exit(EXIT_FAILURE);
    }
    mode_t type = sb.st_mode & S_IFMT;

    if (type == S_IFDIR) {
      DIR * dir;
      struct dirent * ent;
      if ((dir = opendir(path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
          if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
          }
          if (strcmp(ent->d_name, arg) == 0) {
            return true;
          }
        }
        closedir(dir);
      }
    }
    return false;
  }
};

/*Split PATH*/
std::vector<std::string> Shell::splitPath() {
  std::vector<std::string> pathVector;
  size_t first = 0;
  size_t last = ECE551PATH.find(':');
  while (last != std::string::npos) {  //npos is a value which denotes the max index + 1
    pathVector.push_back(ECE551PATH.substr(first, last - first));
    first = last + 1;
    last = ECE551PATH.find(':', last + 1);  //find next ':'
  }
  if (first != ECE551PATH.length()) {  //add last path to vector
    pathVector.push_back(ECE551PATH.substr(first));
  }
  return pathVector;
}

/*Parse $VAR with value*/
std::string Shell::parseVar(std::string str) {
  size_t first = str.find('$');
  size_t last = first + 1;
  while (first != std::string::npos) {
    if (str[first - 1] == '\\') {  //if we have '\' before '$', just ignore and find next '$'
      first = str.find('$', first + 1);
    }
    while (last != std::string::npos) {
      char checkarg = str[last];
      if ((checkarg >= 'A' && checkarg <= 'Z') || (checkarg >= 'a' && checkarg <= 'z') ||
          (checkarg >= '0' && checkarg <= '9') || checkarg == '_') {
        ++last;
      }
      else {
        break;
      }
    }
    if (last == first + 1) {  //ignore var with 0 size
      first = str.find('$', first + 1);
      last = first + 1;
    }
    else {
      std::string var = str.substr(first + 1, last - first - 1);
      auto it = localVar.find(var);
      if (it != localVar.end()) {  //var is in localVar, then replace it
        str.replace(first, last - first, localVar[var]);
        first = str.find('$', first + localVar[var].length());
      }
      else {  //var is not in localVal, then erase it
        str.erase(first, last - first);
        first = str.find('$', first);
      }
      last = first + 1;
    }
  }
  size_t slashPos = str.find("\\$");  //delete '\' in "\$"
  while (slashPos != std::string::npos) {
    str.erase(slashPos, 1);
    slashPos = str.find("\\$", slashPos + 2);
  }
  return str;
}

/*Split Command Line*/
std::vector<std::string> Shell::splitLine(std::string str) {
  std::vector<std::string> argvString;
  size_t first = 0;
  size_t last = 0;
  bool inQuotes = false;  //record the state for in or out of quotes
  while (last <= str.length()) {
    //std::cout << str[last] << " " << inQuotes << "\n";
    if (inQuotes == false) {  //out of quotes
      if (str[last] == '\\') {
        str.erase(last, 1);
        ++last;
      }
      else if (str[last] == '\"') {
        inQuotes = true;
        str.erase(last, 1);
      }
      else if (str[last] == ' ') {
        if (first == last) {
          ++first;
          ++last;
        }
        else {
          std::string tmpArg = str.substr(first, last - first);
          argvString.push_back(tmpArg);
          first = last + 1;
          last = first;
        }
      }
      else {
        ++last;
      }
    }
    else {  //in quotes
      if (str[last] == '\\' && str[last + 1] == '\\') {
        str.erase(last, 1);
        ++last;
      }
      else if (str[last] == '\\' && str[last + 1] == '\"') {
        str.erase(last, 1);
        ++last;
      }
      else if (str[last] == '\"') {
        inQuotes = false;
        str.erase(last, 1);
      }
      else {
        ++last;
      }
    }
  }
  if (inQuotes == true) {  //unclosed quote
    throw std::invalid_argument("Quotes Error");
  }
  if (first != str.length()) {
    argvString.push_back(str.substr(first));
  }

  return argvString;
}

/*Change Directory*/
void Shell::myCd(std::vector<std::string> argvString) {
  for (size_t i = 0; i < argvString.size(); ++i) {  //convert string to char*
    argv.push_back(const_cast<char *>(argvString[i].c_str()));
  }
  argv.push_back(NULL);

  if (argv.size() > 2 + 1) {  //plus "NULL" in the end
    std::cerr << argv[0] << ": too many arguments" << std::endl;
  }
  else if (argv.size() == 2 + 1 && strcmp(argv[1], "~")) {
    if (chdir(argv[1]) != 0) {
      std::cerr << argv[1] << ": No such file or directory" << std::endl;
    }
  }
  else {  //change path to HOME
    chdir(getenv("HOME"));
  }
  argv.clear();
}

/*Set Local Varable*/
void Shell::mySet(std::vector<std::string> argvString, std::string line) {
  if (argvString.size() >= 2) {
    char * checkarg = const_cast<char *>(argvString[1].c_str());  //check if var is valid
    while (*checkarg) {
      if ((*checkarg >= 'A' && *checkarg <= 'Z') || (*checkarg >= 'a' && *checkarg <= 'z') ||
          (*checkarg >= '0' && *checkarg <= '9') || *checkarg == '_') {
        checkarg++;
      }
      else {
        break;
      }
    }
    if (*checkarg) {
      std::cerr << "Invalid var name" << std::endl;
    }
    else {
      if (argvString.size() == 2) {  //if no value, set value to ""
        localVar[const_cast<char *>(argvString[1].c_str())] = "";
      }
      else {  //find the third argument, add the rest of line into value
        std::string lineString = parseVar(line);
        size_t start = lineString.find("set") + 3;
        int argcount = 1;
        while (argcount < 3 && start < lineString.length() - 1) {
          if (lineString[start] == ' ' && lineString[start + 1] != ' ') {
            ++argcount;
          }
          ++start;
        }
        lineString = lineString.substr(start);
        localVar[const_cast<char *>(argvString[1].c_str())] =
            const_cast<char *>(lineString.c_str());
        //std::cout << argv[1] << " " << localVar[argv[1]] << std::endl;
      }
    }
  }
  else {
    std::cerr << argvString[0] << ": Wrong numbers of the arguments" << std::endl;
  }
}

/*Export Into The Environment*/
void Shell::myExport(std::vector<std::string> argvString) {
  for (size_t i = 0; i < argvString.size(); ++i) {  //convert string to char*
    argv.push_back(const_cast<char *>(argvString[i].c_str()));
  }
  argv.push_back(NULL);

  if (argv.size() != 2 + 1) {  //plus "NULL" in the end
    std::cerr << argv[0] << ": need two arguments" << std::endl;
  }
  else {
    auto search = localVar.find(argv[1]);
    if (search != localVar.end()) {
      if (setenv(argv[1], (localVar[argv[1]]).c_str(), 1) != 0) {  //set variable to the environment
        std::cerr << "Could not export" << std::endl;
      }
    }
    else {
      std::cerr << "Could not find " << argv[1] << " in local varibles" << std::endl;
    }
  }
  envp = environ;
  argv.clear();
}

/*Reverse The String*/
void Shell::myRev(std::vector<std::string> argvString) {
  for (size_t i = 0; i < argvString.size(); ++i) {  //convert string to char*
    argv.push_back(const_cast<char *>(argvString[i].c_str()));
  }
  argv.push_back(NULL);

  if (argv.size() != 2 + 1) {  //plus "NULL" in the end
    std::cerr << argv[0] << ": need two arguments" << std::endl;
  }
  else {
    std::string argvString(argv[1]);
    auto search = localVar.find(argvString);
    if (search != localVar.end()) {                                       //find var in locarVar
      reverse(localVar[argvString].begin(), localVar[argvString].end());  //reverse string
    }
    else {
      std::cerr << "Could not find var" << std::endl;
    }
  }
  argv.clear();
}
