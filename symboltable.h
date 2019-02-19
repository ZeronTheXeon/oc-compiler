/**
 * From TA folder 2016.
 */

#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <string>
#include <bitset>
#include <unordered_map>

#include "astree.h"

using namespace std;

attr to_typeattr(int symbol);
attr to_attr(const string *attrib);
const string to_string(attr attribute);
void scanAST(astree *node, FILE *outFile = nullptr);
void performTypeCheck(astree *node);
void processASTNode(astree *node);
void processArrow(astree *node);
void processCall(astree *node);
void processFunction(astree *node);
void processBlock(astree *node);
void processParams(symbol *func, astree *node);
void processIndex(astree *node);
void processVarDecl(astree *node);
symbol_entry processIdent(attr attrib, astree *node);
void processStruct(astree *node);
void processVoid(astree *node);
void printToOutput(symbol_entry sym);
void typeCheckVarDecl(astree *node);
void printDebugAttribsByAttr(attr_bitset attrBitset);
int doesIdentExistInSymtables(astree *node);
void typeCheckIdent(astree *node);
void typeCheckReturn(astree *node);
void processTypeID(astree *node);
void typeCheckUnary(astree *node);
void typeCheckNew(astree *node);
void typeCheckNewStr(astree *node);
void typeCheckNewArray(astree *node);
void typeCheckComparison(astree *node);
void typeCheckBinOp(astree *node);

void printDebugAttribs(astree *node);
void printAttribsToOutFile(FILE *outFile,
                           attr_bitset attribs,
                           location declLoc,
                           const string *type,
                           bool typeQuotes);
void deleteSym(symbol *sym);
void deleteSymTable(symbol_table *table);
void cleanupSymtables();
#endif
