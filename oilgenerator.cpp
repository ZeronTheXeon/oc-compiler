//
// Created by sahjain on 11/29/18.
//

#include <sstream>
#include <iostream>
#include "oilgenerator.h"

string INDENT;

vector<astree *> structDefines, stringConstants, globalVars, functions;
unordered_map<const string *, string> *localNames = nullptr;
unordered_map<const string *, string> globalFuncs;
FILE *oilOutputFile;
int globalVirtRegNum = 1;

void printToOutput(const string &out) {
  fputs(out.c_str(), oilOutputFile);
}

void printIndented(const string &out) {
  string toOut = INDENT;
  toOut += out;
  printToOutput(toOut);
}

char getTypeCharacter(astree *node, bool isArray = false, bool
isIndirect = false) {
  if (node->attributes[unsigned(attr::ARRAY)] || isArray) {
    return 'p';
  } else if (node->attributes[unsigned(attr::STRING)]) {
    return 's';
  } else if (isIndirect) {
    return 'a';
  } else {
    return 'i';
  }
}

string getTypeString(astree *node, bool withSpace = true) {
  string type;
  if (node->attributes[unsigned(attr::STRING)]) {
    type += "char*";
  } else if (node->attributes[unsigned(attr::STRUCT)]) {
    type += "struct " + *node->type + "*";
  } else if (node->attributes[unsigned(attr::VOID)]) {
    type += "void";
  } else if (node->attributes[unsigned
      (attr::INT)]) {
    type += "int";
  } else {
    DEBUGF('o', "Not able to process symbol %s! %d was its "
                "symbol!\n", node->lexinfo->c_str(), node->symbol);
  }

  if (node->attributes[unsigned(attr::ARRAY)]) {
    type += "*";
  }

  if (withSpace) {
    type += " ";
  }

  return type;
}

bool isOperand(astree *node) {
  return node->attributes[unsigned(attr::LVAL)] || node->symbol ==
      TOK_IDENT || node->symbol == TOK_STRINGCON || node->symbol ==
      TOK_INTCON || node->symbol == TOK_CHARCON;
}

string getVirtualRegister(char override = '\0') {
  ostringstream outputName;
  if (override != '\0') {
    outputName << override;
  } else {
    outputName << 'i';
  }
  outputName << globalVirtRegNum++;
  return outputName.str();
}

string mangleName(astree *node, NameType type, const string &override
= "") {

  if (*node->lexinfo == "__FUNC__") {
    return "__func__";
  }

  ostringstream outputName;
  switch (type) {
    case NOTYPE: [[fallthrough]];
    case VARNAME: {
      if (localNames->count(node->lexinfo)) {
        outputName << localNames->at(node->lexinfo);
      } else if (globalFuncs.count(node->lexinfo)) {
        outputName << globalFuncs.at(node->lexinfo);
      } else {
        return mangleName(node, LOCALNAME, override);
      }
      break;
    }
    case GLOBALNAME: {
      outputName << *node->lexinfo;
      break;
    }
    case LOCALNAME: {
      if (localNames->count(node->lexinfo)) {
        outputName << localNames->at(node->lexinfo);
        break;
      }
      outputName << "_" << node->block_nr << "_" << *node->lexinfo;
      if (!localNames->count(node->lexinfo)) {
        localNames->insert(make_pair(node->lexinfo, outputName.str()));
      }
      break;
    }
    case STMTLABEL: {
      if (!override.empty()) {
        outputName << override;
      } else {
        outputName << *node->lexinfo;
      }
      outputName << "_"
                 << node->lloc.filenr << "_"
                 << node->lloc.linenr << "_"
                 << node->lloc.offset;
      break;
    }
    case STRUCTURETYPEIDS: {
      outputName << *node->lexinfo;
      break;
    }
    case STRUCTUREFIELDS: {
      outputName << *node->type << "_" << *node->lexinfo;
      break;
    }
    case VIRTUALREGISTERS: {
//      if (!localNames->count(node->lexinfo) && node->lexinfo !=
//          nullptr) {
        outputName << getTypeCharacter(node) << globalVirtRegNum++;
//        localNames->insert(make_pair(node->lexinfo,
//                                     outputName.str()));
        node->assignedReg = outputName.str();
//      } else {
//        outputName << localNames->at(node->lexinfo);
//      }
      break;
    }
  }

  return outputName.str();
}

/**
 * Utilizes lexinfo of current node.
 * @param node      Node to act on.
 * @param type      Type to mangle to.
 */
void mangleAndPrintName(astree *node, NameType type, const string &
override
= "") {
  printToOutput(mangleName(node, type, override));
}

void printTypeBySymbol(astree *node) {
  printToOutput(getTypeString(node));
}

void printSymbol(astree *node, NameType type) {
  DEBUGF('p', "processing in printSymbol %s to output!\n",
         node->lexinfo->c_str());
  printTypeBySymbol(node);
  if (node->attributes[unsigned(attr::ARRAY)]) {
    mangleAndPrintName(node->children[1], type);
  } else {
    mangleAndPrintName(node->children[0], type);
  }
}

void printStructToOut(astree *node) {
  printToOutput("struct " + *node->type + " {\n");
  if (node->children.size() > 1) {
    for (auto child : node->children[1]->children) {
      printIndented("");
      printTypeBySymbol(child);
      if (child->children.size() > 1) {
        printToOutput(*node->type + "_" + *child->children[1]->lexinfo);
      } else {
        printToOutput(*node->type + "_" + *child->children[0]->lexinfo);
      }
      printToOutput(";\n");
    }
  }
  printToOutput("};\n");
}

/*
|  TOK_VARDECL "=" 7.66.9
|  |  TOK_ARRAY "[]" 7.66.4
|  |  |  TOK_INT "int" 7.66.1
|  |  |  TOK_DECLID "b" 7.66.7
|  |  TOK_NULL "null" 7.66.11
|  TOK_VARDECL "=" 7.67.7
|  |  TOK_INT "int" 7.67.1
|  |  |  TOK_DECLID "d" 7.67.5
|  |  TOK_NULL "null" 7.67.9
|  TOK_VARDECL "=" 7.25.16
|  |  TOK_TYPEID "stack" 7.25.4
|  |  |  TOK_DECLID "stack" 7.25.10
|  |  TOK_NEW "new" 7.25.18
|  |  |  TOK_TYPEID "stack" 7.25.22
*/
void printGlobalVarDeclToOut(astree *node) {
//  printSymbol(node->children[0], GLOBALNAME);
//  printToOutput(";\n");
  printSymbol(node->children[0], GLOBALNAME);
  printToOutput(" = " + procStatement(node->children[1]) + ";\n");
}

void printGlobalStringDeclToOut(astree *node) {
  printSymbol(node->children[0], GLOBALNAME);
  printToOutput(" = " + procStatement(node->children[1]) + ";\n");
}

void printParametersToOut(astree *node) {
  if (!node->children.empty()) {
    printToOutput("(\n");
    for (uint i = 0; i < node->children.size(); i++) {
      auto child = node->children[i];
      printIndented("");
      printSymbol(child, LOCALNAME);
      if (i + 1 == node->children.size()) {
        printToOutput(")\n");
      } else {
        printToOutput(",\n");
      }
    }
  } else {
    printToOutput("(void)\n");
  }
}

void printIfToOut(astree *node) {
//  mangleAndPrintName(node, STMTLABEL);
//  printToOutput(":;\n");

  if (!isOperand(node->children[0])) {
    node->assignedReg = getVirtualRegister();
    printIndented("int " + node->assignedReg + " = "
                      + procStatement(node->children[0]) + ";\n");
  }

  printIndented("if (!");
  if (!isOperand(node->children[0])) {
    printToOutput(node->assignedReg + ") goto ");
  } else {
    printToOutput(procStatement(node->children[0]) + ") goto ");
  }
  if (node->children.size() > 2) {
    mangleAndPrintName(node, STMTLABEL, "else");
  } else {
    mangleAndPrintName(node, STMTLABEL, "fi");
  }
  printToOutput(";\n");

  procStatement(node->children[1]); // consequent

  if (node->children.size() > 2) {
    printIndented("goto ");
    mangleAndPrintName(node, STMTLABEL, "fi");
    printToOutput(";\n");

    mangleAndPrintName(node, STMTLABEL, "else");
    printToOutput(":;\n");
    procStatement(node->children[2]); // alternate
  }

  mangleAndPrintName(node, STMTLABEL, "fi"); // end
  printToOutput(":;\n");
}

void printWhileToOut(astree *node) {
  mangleAndPrintName(node, STMTLABEL);
  printToOutput(":;\n");

  if (isOperand(node->children[0])) {
    printIndented("if (!");
    if (node->children[0]->attributes[unsigned(attr::LVAL)]) {
      mangleAndPrintName(node->children[0], LOCALNAME);
    } else {
      mangleAndPrintName(node->children[0], NOTYPE);
    }
    printToOutput(") goto ");
  } else {
    node->assignedReg = getVirtualRegister();
    printIndented("int " + node->assignedReg + " = " + procStatement
        (node->children[0]) + ";\n");
    printIndented("if (!" + node->assignedReg + ") goto ");
  }
  mangleAndPrintName(node, STMTLABEL, "break");
  printToOutput(";\n");

  procStatement(node->children[1]); // process block

  printIndented("goto ");
  mangleAndPrintName(node, STMTLABEL);
  printToOutput(";\n");
  mangleAndPrintName(node, STMTLABEL, "break");
  printToOutput(":;\n");
}

void printFunctionToOut(astree *node) {
  delete localNames;
  localNames = new unordered_map<const string *, string>;
  printSymbol(node->children[0], NOTYPE);
  printParametersToOut(node->children[1]);
  printToOutput("{\n");
  procStatement(node->children[2]);
  printToOutput("}\n");
}

string procStatement(astree *node) {
  switch (node->symbol) {
    case TOK_BLOCK: {
      for (auto child : node->children) {
        procStatement(child);
        printToOutput("\n");
      }
      break;
    }
    case TOK_CALL: {
      vector<string> arguments;

      for (uint i = 1; i < node->children.size(); i++) {
        arguments.push_back(procStatement(node->children[i]));
      }

      string ourArgs;

      if (node->parent->symbol == TOK_VARDECL) {
        ourArgs = mangleName(node->children[0], VARNAME) +
            "(";
        if (!arguments.empty()) {
          ourArgs += arguments[0]; // first arg
          for (uint i = 2; i < node->children.size(); i++) {
            ourArgs += ", ";
            ourArgs += arguments[i - 1];
          }
        }

        ourArgs += ")";
        return ourArgs;
      } else {
        // Create a virtual register if needed
        if (node->children[0]->attributes[unsigned(attr::VOID)]) {
          printIndented(mangleName(node->children[0], VARNAME) +" (");
        } else {
          ourArgs = mangleName(node, VIRTUALREGISTERS);
          printIndented(getTypeString(node->children[0]) +
              ourArgs + " = " + mangleName(node->children[0],
                  VARNAME) + " (");
        }

        if (!arguments.empty()) {
          printToOutput(arguments[0]);
          for (uint i = 2; i < node->children.size(); i++) {
            printToOutput(", " + arguments[i - 1]);
          }
        }

        printToOutput(");\n");
        return ourArgs;
      }
    }
    case TOK_VARDECL: {
      printIndented(getTypeString(node->children[0]) +
          (node->children[0]->attributes[unsigned(attr::ARRAY)] ?
           mangleName(node->children[0]->children[1], LOCALNAME) :
           mangleName(node->children[0]->children[0], LOCALNAME))
                        + " = " +
          procStatement(node->children[1]) + ";\n");
      break;
    }
    case TOK_IF: {
      printIfToOut(node);
      break;
    }
    case TOK_WHILE: {
      printWhileToOut(node);
      break;
    }
    case TOK_RETURN: {
      if (node->children.empty()) {
        printIndented("return;\n");
      } else {
        printIndented("return " + procStatement(node->children[0]) +
            ";\n");
      }
      break;
    }
    case TOK_IDENT: {
      return mangleName(node, LOCALNAME);
    }
    case TOK_FIELD: {
      mangleAndPrintName(node, STRUCTUREFIELDS);
      break;
    }
    case TOK_ARROW: {
      string regName;
      regName = getVirtualRegister('a');
      printIndented(getTypeString(node) + regName + " = " +
          procStatement(node->children[0]) + "->" +
          *node->children[0]->type + "_" +
          *node->children[1]->lexinfo + ";\n");
      return regName;
    }
    case TOK_NEW: {
      return "xcalloc (1, sizeof(struct " + *node->children[0]->type
          + "))";
    }
    case TOK_NEWARRAY: {
      return "xcalloc (" + procStatement(node->children[1]) +
          ", sizeof(" + getTypeString(node->children[0], false) + "))";
    }
    case TOK_NEWSTR: {
      return ""; // TODO(sahjain)
    }
    case TOK_NOT: {
      // We do not want NOT
      return "!" + procStatement(node->children[0]);
    }
    case TOK_POS: [[fallthrough]];
    case TOK_NEG: {
      return *node->lexinfo + procStatement(node->children[0]);
    }
    case TOK_NE: [[fallthrough]];
    case TOK_EQ: [[fallthrough]];
    case TOK_LT: [[fallthrough]];
    case TOK_LE: [[fallthrough]];
    case TOK_GT: [[fallthrough]];
    case TOK_GE: [[fallthrough]];
    case '+': [[fallthrough]];
    case '-': [[fallthrough]];
    case '*': [[fallthrough]];
    case '/': [[fallthrough]];
    case '%': {
      string regName = mangleName(node, VIRTUALREGISTERS);
      printIndented(
          "int " + regName + " = " + procStatement(node->children[0])
              + " " +
              *node->lexinfo + " " + procStatement(node->children[1])
              + ";\n");
      return regName;
    }
    case '=': {
      printIndented(procStatement(node->children[0]) + " "
                        + *node->lexinfo + " "
                        + procStatement(node->children[1]) + ";\n");
      return "";
    }
    case TOK_INDEX: {
      string regName = getVirtualRegister('a');
      string type = getTypeString(node);

      printIndented(type + regName + " = "
                        + procStatement(node->children[0]) + "["
                        + procStatement
                            (node->children[1]) + "];\n");
      return regName;
    }
    case TOK_NULL: {
      return "0";
    }
    case TOK_STRINGCON: [[fallthrough]];
    case TOK_CHARCON: [[fallthrough]];
    case TOK_INTCON: {
      return *node->lexinfo;
    }
    case ';': {
      return ""; // noprint
    }
    default: {
      DEBUGF('o', "%s with token code %d is unhandled! :)",
             node->lexinfo->c_str(), node->symbol);
      break;
    }
  }
  DEBUGF('o', "Printing COULD NOT PROCESS FOR %s with token code %d!\n",
         node->lexinfo->c_str(), node->symbol);
  return "";
}

void appendTopLevel(astree *node) {
  switch (node->symbol) {
    case TOK_STRUCT: {
      structDefines.push_back(node);
      break;
    }
    case TOK_VARDECL: {
      if (node->children[0]->symbol == TOK_STRING) {
        stringConstants.push_back(node);
      } else {
        globalVars.push_back(node);
      }
      break;
    }
    case TOK_FUNCTION: {
      functions.push_back(node);
      [[fallthrough]];
    }
    case TOK_PROTO: {
      if (node->children[0]->attributes[unsigned(attr::ARRAY)]) {
        globalFuncs.insert(make_pair
                               (node->children[0]
                               ->children[1]->lexinfo,
                                *node->children[0]
                                ->children[1]->lexinfo));
      } else {
        globalFuncs.insert(make_pair
                               (node->children[0]
                               ->children[0]->lexinfo,
                                *node->children[0]
                                ->children[0]->lexinfo));
      }
      break;
    }
    default: {
      break;
    }
  }
}

void cleanupOil() {
  delete localNames;
}

void startOilGenerator(FILE *outFile) {
  oilOutputFile = outFile;
  INDENT = "        ";
  for (auto &child : parser::root->children) {
    appendTopLevel(child);
  }

  printToOutput("#define __OCLIB_C__\n"
                "#include \"oclib.h\"\n");

  for (auto &child : structDefines) {
    printStructToOut(child);
  }

  for (auto &child : stringConstants) {
    printGlobalStringDeclToOut(child);
  }

  for (auto &child : globalVars) {
    printGlobalVarDeclToOut(child);
  }

  for (auto &child : functions) {
    printFunctionToOut(child);
  }

  cleanupOil();
}
