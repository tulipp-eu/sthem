#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <iostream>
#include <vector>
#include <string>

std::string ext(std::string s) {
  char sep = '.';

  size_t i = s.rfind(sep, s.length());
  if(i != std::string::npos) {
    return(s.substr(i+1, s.length() - i));
  }

  return("");
}

std::string base(std::string s) {
  char sep1 = '/';
  char sep2 = '.';

  size_t i = s.rfind(sep1, s.length());
  size_t j = s.rfind(sep2, s.length()) - 1;

  if((i != std::string::npos) && (j != std::string::npos)) {
    return(s.substr(i+1, j - i));
  } else if((i != std::string::npos) && (j == std::string::npos)) {
    return(s.substr(i+1, s.length() - i));
  } else if((i == std::string::npos) && (j != std::string::npos)) {
    return(s.substr(0, j+1));
  } else if((i == std::string::npos) && (j == std::string::npos)) {
    return(s);
  }
}

int main(int argc, char *argv[]) {
  std::vector<std::string> args;

  std::string input;
  std::string output;
  std::string optlevel;

  bool instrument = false;

  int arg = 6;

  if(argc <= arg) {
    printf("Usage: ...\n");
  }

  while(arg < argc) {
    std::string argstring = std::string(argv[arg]);

    if(ext(argstring) == std::string("c")) {
      input = argstring;
      arg++;
    } else if(argstring == std::string("-o")) {
      output = std::string(argv[arg+1]);
      arg += 2;
    } else if(argstring.substr(0,2) == std::string("-O")) {
      optlevel = argstring;
      arg++;
    } else if(argstring == std::string("--tulipp-instrument")) {
      instrument = true;
      arg++;
    } else {
      args.push_back(std::string(argv[arg]));
      arg++;
    }
  }

  std::string clangline = std::string(argv[1]) + " ";
  for(auto arg : args) {
    clangline += arg + " ";
  }
  clangline += "-Os -target aarch64--none-gnueabi -g -emit-llvm -S " + input;
  std::string parserline1 = std::string(argv[2]) + " " + base(input) + ".ll -xml " + base(input) + ".xml";
  std::string parserline2 = std::string(argv[2]) + " " + base(input) + ".ll -ll " + base(input) + "_2.ll";
  if(instrument) parserline2 += " --instrument";
  std::string optline = std::string(argv[3]) + " " + optlevel + " " + base(input) + "_2.ll -o " + base(input) + "_3.ll";
  if(optlevel == std::string("-Os")) optlevel = std::string("-O2");
  std::string llcline = std::string(argv[4]) + " " + optlevel + " -mcpu=cortex-a53 " + base(input) + "_3.ll -o " + base(input) + ".s";
  std::string asline = std::string(argv[5]) + " -mcpu=cortex-a53 " + base(input) + ".s -o " + output;

  if(!system(clangline.c_str())) {
    if(!system(parserline1.c_str())) {
      if(!system(parserline2.c_str())) {
        if(!system(optline.c_str())) {
          if(!system(llcline.c_str())) {
            if(!system(asline.c_str())) {
              return 0;
            }
          }
        }
      }
    }
  }

  return 1;
}
