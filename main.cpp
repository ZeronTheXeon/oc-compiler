/**
 * Sanjeet Jain
 * sahjain
 */

#include <iostream>
#include <getopt.h>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include "auxlib.h"
#include "string_set.h"
#include "lyutils.h"
#include "symboltable.h"
#include "oilgenerator.h"

string CPP;

/**
 * Sets initial variable values.
 */
void init() {
  CPP = "/usr/bin/cpp -nostdinc ";
}

/**
 * Verifies file's extension is oc and returns the extensionless
 * filename if successful.
 *
 * @param extensionName     Name/path of file with extension.
 * @return                  Name of file without extension.
 */
string verifyFileName(const char *extensionName) {
  const string fileName = string(basename(extensionName));
  size_t i = fileName.rfind('.', fileName.length());

  if (i != string::npos &&
      fileName.substr(i + 1, fileName.length() - i) == "oc") {
    return fileName.substr(0, i);
  }

  errprintf("Invalid file given! Please only try to "
            "compile .oc files!");
  exit(exec::exit_status);
}

/**
 * Processes arguments from the command line.
 *
 * @param argc      Number of arguments given.
 * @param argv      2D char array of arguments.
 * @return          Location of file on the command line.
 */
int scanArgs(int argc, char *argv[]) {
  int c;
  while ((c = getopt(argc, argv, "ly@:D:")) != -1) {
    switch (c) {
      case 'l': {
        yy_flex_debug = 1;
        continue;
      }
      case 'y': {
        yydebug = 1;
        continue;
      }
      case '@': {
        set_debugflags(optarg);
        continue;
      }
      case 'D': {
        ostringstream stream;
        stream << CPP << "-D " << optarg << " ";
        CPP = stream.str();
        continue;
      }
      default: {
        errprintf("Unrecognized option %c!", c);
        continue;
      }
    }
  }

  // By this point, optind will be after the options, probably the file
  if (optind >= argc) {
    errprintf("SYNOPSIS\n"
              "oc [-ly] [-@ flag...]  [-D string] program.oc\n"
              "OPTIONS\n"
              "-@flags\t"
              "Sets Debug flags, currently unused!\n"
              "-Dstring\t"
              "These options are passed to the C PreProcessor.\n"
              "-l\t"
              "Debug yylex() with yy_flex_debug = 1\n"
              "-y\t"
              "Debug yyparse() with yydebug = 1\n");
    exit(exec::exit_status);
  }

  return optind;
}

int main(int argc, char *argv[]) {
  exec::execname = argv[0];
  if (argc < 2) {
    errprintf("SYNOPSIS\n"
              "oc [-ly] [-@ flag...]  [-D string] program.oc\n"
              "OPTIONS\n"
              "-@flags\t"
              "Sets Debug flags, currently unused!\n"
              "-Dstring\t"
              "These options are passed to the C PreProcessor.\n"
              "-l\t"
              "Debug yylex() with yy_flex_debug = 1\n"
              "-y\t"
              "Debug yyparse() with yydebug = 1\n");
    exit(exec::exit_status);
  }

  init();

  int fileIndex = scanArgs(argc, argv);
  string fileName = verifyFileName(argv[fileIndex]);

  ostringstream stream;
  stream << CPP << argv[fileIndex];
  CPP = stream.str();

  yyin = popen(CPP.c_str(), "r");
  lexer::tokenFile = fopen((fileName + ".tok").c_str(), "w");

  int parse_rc = 0;

  if (yyin == nullptr) {
    errprintf("%s: %s: %s\n", exec::execname.c_str(), CPP.c_str(),
              strerror(errno));
    exit(exec::exit_status);
  } else if (lexer::tokenFile == nullptr) {
    errprintf("Could not open pipe to tokenfile output file!");
  } else {
    if (yy_flex_debug) {
      fprintf(stderr, "-- popen (%s), fileno(yyin) = %d\n",
              CPP.c_str(), fileno(yyin));
    }
    lexer::newfilename(fileName + ".oc");
    parse_rc = yyparse();
    int pclose_rc = pclose(yyin);

    eprint_status(CPP.c_str(), pclose_rc);
    if (pclose_rc != 0) {
      exec::exit_status = EXIT_FAILURE;
    }
  }

  FILE *tokenTreeFile = fopen((fileName + ".ast").c_str(), "w");
  FILE *symbolTableFile = fopen((fileName + ".sym").c_str(), "w");
  FILE *intermediateFile = fopen((fileName + ".oil").c_str(), "w");
  if (tokenTreeFile == nullptr) {
    errprintf("Could not open pipe to parser output file!");
  } else if (parse_rc) {
    errprintf("Could not parse file! Error code %d\n", parse_rc);
  } else if (symbolTableFile == nullptr) {
    errprintf("Could not open pipe to symbol table output file!");
  } else {
    scanAST(parser::root, symbolTableFile);
    astree::print(tokenTreeFile, parser::root, 0);
    startOilGenerator(intermediateFile);
    fclose(tokenTreeFile);
    fclose(symbolTableFile);
  }

  delete parser::root;
//  cleanupSymtables();

  FILE *stringsetFile = fopen((fileName + ".str").c_str(), "w");

  if (stringsetFile == nullptr) {
    errprintf("Could not open pipe to stringset output file!");
  } else {
    string_set::dump(stringsetFile);
    fclose(stringsetFile);
  }

  fclose(lexer::tokenFile);
  yylex_destroy();

  return exec::exit_status;
}
