/**
 * From TA folder 2016.
 */

// Import the relevant STL classes
#include <bitset>
#include <iostream>
#include <string>
#include <unordered_map>

#include "symboltable.h"
#include "lyutils.h"

using namespace std;

size_t next_block = 1, localSeq = 0;
auto structDefs = new symbol_table, globalDefs = new symbol_table;
symbol_table *localDefs = nullptr;
FILE *outputFile;
bool firstBracePlaced = false;

attr to_typeattr(int symbol) {
  const unordered_map<int, attr> hash{
      {TOK_VOID, attr::VOID},
      {TOK_INT, attr::INT},
      {TOK_NULL, attr::NULLX},
      {TOK_STRING, attr::STRING},
      {TOK_STRUCT, attr::STRUCT},
      {TOK_TYPEID, attr::TYPEID},
      {TOK_FUNCTION, attr::FUNCTION},
      {TOK_PROTO, attr::FUNCTION},
      {TOK_ARRAY, attr::ARRAY},
      {TOK_FIELD, attr::FIELD},
      {TOK_TYPEID, attr::TYPEID}
  };
  auto str = hash.find(symbol);
  if (str == hash.end()) {
    return attr::BITSET_SIZE;
//    throw invalid_argument(string(__PRETTY_FUNCTION__) + ": "
//                               + to_string(unsigned(symbol)));
  }
  DEBUGF('v', "returning %s from to_typeattr\n", to_string
      (str->second).c_str());
  return str->second;
}

attr to_attr(const string *attrib) {
  DEBUGF('v', "Entering to_attr with %s!\n", attrib->c_str());
  const unordered_map<string, attr> hash{
      {"void", attr::VOID},
      {"int", attr::INT},
      {"null", attr::NULLX},
      {"string", attr::STRING},
      {"struct", attr::STRUCT},
      {"array", attr::ARRAY},
      {"function", attr::FUNCTION},
      {"variable", attr::VARIABLE},
      {"field", attr::FIELD},
      {"typeid", attr::TYPEID},
      {"param", attr::PARAM},
      {"local", attr::LOCAL},
      {"lval", attr::LVAL},
      {"const", attr::CONST},
      {"vreg", attr::VREG},
      {"vaddr", attr::VADDR},
      {"bitset_size", attr::BITSET_SIZE},
  };
  auto str = hash.find(*attrib);
  if (str == hash.end()) {
    return attr::BITSET_SIZE;
//    throw invalid_argument(string(__PRETTY_FUNCTION__) + ": "
//                               + *attrib);
  }
  return str->second;
}

const string to_string(attr attribute) {
  const unordered_map<attr, string> hash{
      {attr::VOID, "void"},
      {attr::INT, "int"},
      {attr::NULLX, "null"},
      {attr::STRING, "string"},
      {attr::STRUCT, "struct"},
      {attr::ARRAY, "array"},
      {attr::FUNCTION, "function"},
      {attr::VARIABLE, "variable"},
      {attr::FIELD, "field"},
      {attr::TYPEID, "typeid"},
      {attr::PARAM, "param"},
      {attr::LOCAL, "local"},
      {attr::LVAL, "lval"},
      {attr::CONST, "const"},
      {attr::VREG, "vreg"},
      {attr::VADDR, "vaddr"},
      {attr::BITSET_SIZE, "bitset_size"},
  };
  auto str = hash.find(attribute);
  if (str == hash.end()) {
    throw invalid_argument(string(__PRETTY_FUNCTION__) + ": "
                               + to_string(unsigned(attribute)));
  }
  return str->second;
}

void makeParents(astree *node) {
  for (auto child : node->children) {
    child->parent = node;
    makeParents(child);
  }
}

void scanAST(astree *node, FILE *outFile) {
  if (node == nullptr) {
    return;
  }

  if (outFile != nullptr) {
    outputFile = outFile;
  }

  if (node != parser::root) {
    processASTNode(node);
  } else {
    DEBUGF('v', "Making parents!\n");
    makeParents(parser::root);
  }

  for (auto &child : node->children) {
    scanAST(child);
  }

  performTypeCheck(node);
}

void setSymbolAttributes(astree *node, symbol *sym, attr_bitset attrs =
-1) {
  if (attrs != -1) {
    sym->attributes = attrs;
  } else {
    sym->attributes = node->attributes;
  }
  auto unsignit = to_attr(node->lexinfo);
  if (unsignit != attr::BITSET_SIZE) {
    if (unsignit == attr::TYPEID) {
      sym->attributes[unsigned(attr::STRUCT)] = true;
      sym->type = const_cast<string *>(node->lexinfo);
    } else {
      sym->attributes[unsigned(unsignit)] = true;
    }
  } else {
    DEBUGF('v', "Could not find attribute for node %s!\n",
           node->lexinfo->c_str());
    unsignit = to_typeattr(node->symbol);
    if (unsignit != attr::BITSET_SIZE) {
      if (unsignit == attr::TYPEID) {
        sym->attributes[unsigned(attr::STRUCT)] = true;
        sym->type = const_cast<string *>(node->lexinfo);
      } else {
        sym->attributes[unsigned(unsignit)] = true;
      }
    }
  }
}

symbol *symbolFromNode(astree *node, attr_bitset attrs) {
  auto sym = new symbol;
  sym->lloc = node->lloc;
  sym->type = const_cast<string *>(node->type);
  sym->block_nr = next_block;
  setSymbolAttributes(node, sym, attrs);
  return sym;
}

symbol *symbolFromFunctionNode(astree *node, attr_bitset attrs) {
  symbol *sym;
  if (node->children[0]->symbol == TOK_ARRAY) {
    sym = symbolFromNode(node->children[0]->children[0], attrs);
  } else {
    sym = symbolFromNode(node->children[0], attrs);
  }
  sym->attributes[unsigned(attr::FUNCTION)] = true;
  return sym;
}

void processASTNode(astree *node) {
  switch (node->symbol) {
    case TOK_STRUCT: {
      processStruct(node);
      break;
    }
    case TOK_IDENT: {
      typeCheckIdent(node);
      break;
    }
    case TOK_BLOCK: {
      // TODO(sahjain): Handle block in scanAST();
//      processBlock(node);
//      deleteSymTable(localDefs);
//      localDefs = nullptr;
//      ;
      break;
    }
    case TOK_FUNCTION: {
      [[fallthrough]];
    }
    case TOK_PROTO: {
      processFunction(node);
      deleteSymTable(localDefs);
      localDefs = nullptr;
      break;
    }
    case TOK_VARDECL: {
      processVarDecl(node);
      break;
    }
    default:break;
  }
}

void performTypeCheck(astree *node) {
  switch (node->symbol) {
    case TOK_INDEX: {
      processIndex(node);
      break;
    }
    case TOK_CALL: {
      processCall(node);
      break;
    }
    case TOK_INTCON: {
      node->attributes[unsigned(attr::INT)] = true;
      node->attributes[unsigned(attr::CONST)] = true;
      break;
    }
    case TOK_STRINGCON: {
      node->attributes[unsigned(attr::STRING)] = true;
      node->attributes[unsigned(attr::CONST)] = true;
      break;
    }
    case TOK_CHARCON: {
      node->attributes[unsigned(attr::INT)] = true;
      node->attributes[unsigned(attr::CONST)] = true;
      break;
    }
    case TOK_INT: {
      node->attributes[unsigned(attr::INT)] = true;
      break;
    }
    case TOK_STRING: {
      node->attributes[unsigned(attr::STRING)] = true;
      break;
    }
    case TOK_NULL: {
      node->attributes[unsigned(attr::NULLX)] = true;
      node->attributes[unsigned(attr::CONST)] = true;
      break;
    }
    case TOK_VOID: {
      processVoid(node);
      break;
    }
    case TOK_RETURN: {
      typeCheckReturn(node);
      break;
    }
    case TOK_ARROW: {
      processArrow(node);
      break;
    }
    case TOK_IDENT: {
//      typeCheckIdent(node);
      break;
    }
    case TOK_VARDECL: {
      typeCheckVarDecl(node);
      break;
    }
    case TOK_TYPEID: {
      processTypeID(node);
      break;
    }
    case TOK_POS: {
      [[fallthrough]];
    }
    case TOK_NEG: {
      [[fallthrough]];
    }
    case TOK_NOT: {
      typeCheckUnary(node);
      break;
    }
    case TOK_NEW: {
      typeCheckNew(node);
      break;
    }
    case TOK_NEWSTR: {
      typeCheckNewStr(node);
      break;
    }
    case TOK_NEWARRAY: {
      typeCheckNewArray(node);
      break;
    }
    case TOK_EQ: {
      [[fallthrough]];
    }
    case TOK_NE: {
      [[fallthrough]];
    }
    case TOK_LT: {
      [[fallthrough]];
    }
    case TOK_LE: {
      [[fallthrough]];
    }
    case TOK_GT: {
      [[fallthrough]];
    }
    case TOK_GE: {
      typeCheckComparison(node);
      break;
    }
    case '+': {
      [[fallthrough]];
    }
    case '-': {
      [[fallthrough]];
    }
    case '*': {
      [[fallthrough]];
    }
    case '/': {
      [[fallthrough]];
    }
    case '%': {
      typeCheckBinOp(node);
      break;
    }
    default: {
      break;
    }
  }
}

/**
 * Compare a to b.
 * @param a     Symbol to be compared to.
 * @param b     Symbol to compare a to.
 * @return      True if a matches b, else false.
 */
bool isTypeOkay(symbol *a, symbol *b) {
  if (a == b) {
    return true;
  }

  static vector<attr> nullableTypes{
      attr::STRUCT, attr::STRING, attr::ARRAY
  };

  if (a->attributes[unsigned(attr::NULLX)]) {
    for (auto attrib : nullableTypes) {
      if (b->attributes[unsigned(attrib)]) {
        return true;
      }
    }
  } else if (b->attributes[unsigned(attr::NULLX)]) {
    for (auto attrib : nullableTypes) {
      if (a->attributes[unsigned(attrib)]) {
        return true;
      }
    }
  }

  static vector<attr> attrs{
      attr::VOID, attr::INT, attr::NULLX, attr::STRING, attr::STRUCT,
      attr::ARRAY
  };

  for (auto attrib : attrs) {
    if (a->attributes[unsigned(attrib)] != b->attributes[unsigned
        (attrib)]) {
      DEBUGF('v', "Attributes did not agree on %s!\n", to_string(attrib)
          .c_str());
      return false;
    }
  }

  if (a->attributes[unsigned(attr::STRUCT)]) {
    return a->type == b->type;
  } else {
    return true;
  }
}

void typeCheckBinOp(astree *node) {
  auto leftattrs = node->children[0]->attributes;
  auto rightattrs = node->children[1]->attributes;
  if (!leftattrs[unsigned(attr::INT)] || !rightattrs[unsigned
      (attr::INT)]) {
    errprintf("Cannot operate on incompatible types!\n");
    return;
  }
  node->attributes[unsigned(attr::INT)] = true;
  node->attributes[unsigned(attr::VREG)] = true;
}

void typeCheckComparison(astree *node) {
  auto symA = new symbol, symB = new symbol;
  symA->attributes = node->children[0]->attributes;
  symA->type = const_cast<string *>(node->children[0]->type);
  symB->attributes = node->children[1]->attributes;
  symB->type = const_cast<string *>(node->children[1]->type);

  if (!isTypeOkay(symA, symB)) {
    errprintf("Cannot compare types!\n");
    return;
  }

  node->attributes[unsigned(attr::INT)] = true;
  node->attributes[unsigned(attr::VREG)] = true;
  deleteSym(symA);
  deleteSym(symB);
}

void typeCheckNew(astree *node) {
  auto ident = node->children[0];
  node->type = ident->type;
  node->attributes = ident->attributes;
  node->attributes[unsigned(attr::VREG)] = true;
}

void typeCheckNewStr(astree *node) {
  if (!node->children[1]->attributes[unsigned(attr::INT)]) {
    errprintf("Attempting to create new string with non-int type!\n");
  } else {
    node->attributes[unsigned(attr::STRING)] = true;
    node->attributes[unsigned(attr::VREG)] = true;
  }
}

/*
TOK_NEWARRAY "new" 7.92.24
|  TOK_INT "int" 7.92.28
|  TOK_INTCON "24" 7.92.32
*/
void typeCheckNewArray(astree *node) {
  auto typeattribs = node->children[0]->attributes;
  if (typeattribs[unsigned(attr::ARRAY)]) {
    errprintf("Cannot have more than 1D array!\n");
    return;
  } else if (typeattribs[unsigned(attr::VOID)]) {
    DEBUGF('v', "In incompatible type, attribs of typeattribs:\n");
    printDebugAttribsByAttr(typeattribs);
    errprintf("Using incompatible type for declaring array!\n");
    return;
  }

  node->attributes[unsigned(attr::ARRAY)] = true;
  node->attributes[unsigned(attr::VREG)] = true;

  if (typeattribs[unsigned(attr::STRING)]) {
    node->attributes[unsigned(attr::STRING)] = true;
  } else if (typeattribs[unsigned(attr::INT)]) {
    node->attributes[unsigned(attr::INT)] = true;
  } else if (typeattribs[unsigned(attr::STRUCT)]) {
    node->attributes[unsigned(attr::STRUCT)] = true;
    node->type = node->children[0]->lexinfo;
  }
}

void typeCheckUnary(astree *node) {
  auto unopChild = node->children[0];
  if (!unopChild->attributes[unsigned(attr::INT)] ||
      unopChild->attributes[unsigned(attr::ARRAY)]) {
    errprintf("Cannot use %s unary operator with %s!\n",
              node->lexinfo->c_str(), unopChild->lexinfo->c_str());
  }

  node->attributes[unsigned(attr::INT)] = true;
  node->attributes[unsigned(attr::VREG)] = true;
}

/*
|  TOK_FUNCTION "" 7.68.5
|  |  TOK_ARRAY "[]" 7.68.5
|  |  |  TOK_VOID "void" 7.68.1
|  |  |  TOK_DECLID "f" 7.68.8
|  |  TOK_PARAM "(" 7.68.9
|  |  TOK_BLOCK "{" 7.68.12
|  TOK_PROTO "" 6.29.1
|  |  TOK_VOID "void" 6.29.1
|  |  |  TOK_DECLID "__assert_fail" 6.29.6
|  |  TOK_PARAM "(" 6.29.20
|  |  |  TOK_STRING "string" 6.29.21
|  |  |  |  TOK_DECLID "expr" 6.29.28
|  |  |  TOK_STRING "string" 6.29.34
|  |  |  |  TOK_DECLID "file" 6.29.41
|  |  |  TOK_INT "int" 6.29.47
|  |  |  |  TOK_DECLID "line" 6.29.51
|  |  |  TOK_STRING "string" 6.29.57
|  |  |  |  TOK_DECLID "func" 6.29.64
*/
void typeCheckReturn(astree *node) {
  astree *masterNode = node;
  while (masterNode->symbol != TOK_FUNCTION && masterNode->symbol !=
      TOK_ROOT) {
    masterNode = masterNode->parent;
  }

  DEBUGF('v', "node is %s\n", masterNode->lexinfo->c_str());
  DEBUGF('v', "node firstchild is %s\n",
         masterNode->children[0]->lexinfo->c_str());
  DEBUGF('v', "masterNode attributes are literally:\n");
  printDebugAttribsByAttr(masterNode->attributes);
  DEBUGF('v', "node firstchild firstchild is %s\n",
         masterNode->children[0]->children[0]->lexinfo->c_str());

  if (masterNode->symbol == TOK_ROOT) {
    errprintf("Return is outside of function!\n");
    return;
  } else if (masterNode->attributes[unsigned(attr::VOID)] ^
      node->children.empty()) {
    errprintf("Returning incompatible type from function!\n");
    DEBUGF('b', "Master node has attributes:\n");
    printDebugAttribsByAttr(masterNode->attributes);
    DEBUGF('b', "Children emptiness is %d!\n", node->children
        .empty());
    return;
  }

  if (masterNode->attributes[unsigned(attr::VOID)] &&
      node->children.empty()) {
    return;
  }

  auto symA = new symbol, symB = new symbol;
  auto declNode = masterNode->children[0]->children[0];
  if (masterNode->children[0]->symbol == TOK_ARRAY) {
    declNode = masterNode->children[0]->children[1];
  }

  symA->attributes = masterNode->attributes;
  symA->type = const_cast<string *>(masterNode->type);

  symB->attributes = node->children[0]->attributes;
  symB->type = const_cast<string *>(node->children[0]->type);

  if (!isTypeOkay(symA, symB)) {
    errprintf("Returning incompatible type from function %s!\n",
              declNode->lexinfo->c_str());
    DEBUGF('b', "symA has attributes:\n");
    printDebugAttribsByAttr(symA->attributes);
//    DEBUGF('b', "symA type is %s\n", const_cast<const string *
//        >(symA->type)->c_str());
    DEBUGF('b', "symB has attributes:\n");
    printDebugAttribsByAttr(symB->attributes);
//    DEBUGF('b', "symB type is %s\n", const_cast<const string *
//        >(symB->type)->c_str());
    return;
  }
  // TODO(sahjain): This is not enough, we have to go deeper in our
  // cleaning (clean params, etc).
//  delete symA;
//  delete symB;
}

void processVoid(astree *node) {
  if (!(node->parent->symbol == TOK_FUNCTION
      || node->parent->symbol == TOK_PROTO)) {
    errprintf("Using void in invalid place!!\n");
  } else {
    node->attributes[unsigned(attr::VOID)] = true;
  }
}

void processTypeID(astree *node) {
  if (structDefs->count(const_cast<string *>(node->lexinfo))) {
    node->attributes[unsigned(attr::STRUCT)] = true;
    node->type = node->lexinfo;
  }
}

/*
TOK_INDEX "[" 7.47.24
|  TOK_IDENT "argv" 7.47.20
|  TOK_IDENT "argi" 7.47.25
*/
void processIndex(astree *node) {
  auto ident = node->children[0];
  auto identLex = ident->lexinfo;
  if (!node->children[1]->attributes[unsigned(attr::INT)]) {
    errprintf("Attempting to access ident %s with a non-integer "
              "type!\n", identLex->c_str());
    return;
  }

  auto isIdentArray = ident->attributes[unsigned(attr::ARRAY)];
  auto isIdentString = ident->attributes[unsigned(attr::STRING)];
  if (!isIdentString && !isIdentArray) {
    errprintf("Cannot use array access on ident %s!\n",
              identLex->c_str());
    return;
  }

  if (isIdentString && !isIdentArray) {
    DEBUGF('v', "Ident %s is a string but not an array!\n",
           ident->lexinfo->c_str());
    node->attributes[unsigned(attr::INT)] = true;
  } else {
    node->attributes[unsigned(attr::STRING)] =
        ident->attributes[unsigned(attr::STRING)];
    node->attributes[unsigned(attr::STRUCT)] =
        ident->attributes[unsigned(attr::STRUCT)];
    node->attributes[unsigned(attr::INT)] =
        ident->attributes[unsigned(attr::INT)];
  }

  node->attributes[unsigned(attr::VADDR)] = true;
  node->attributes[unsigned(attr::LVAL)] = true;
}

/*
TOK_ARROW "->" 7.17.16
|  TOK_IDENT "stack" 7.17.11
|  TOK_FIELD "top" 7.17.18
 */
void processArrow(astree *node) {
  if (!node->children[0]->attributes[unsigned(attr::STRUCT)]) {
    errprintf("Left operand is not a struct!\n");
    return;
  }

  auto lexinfo = const_cast<string *>(node->children[0]->type);

  if (!structDefs->count(lexinfo)) {
    errprintf("Could not find struct %s for reference %s!\n",
              node->children[0]->type->c_str(),
              node->children[1]->lexinfo->c_str());
    return;
  }

  auto structSym = structDefs->at(lexinfo);

  auto fieldName = const_cast<string *>(node->children[1]->lexinfo);
  if (!structSym->fields->count(fieldName)) {
    errprintf("Cannot reference nonexistent field %s for struct "
              "%s!\n", fieldName->c_str(), lexinfo->c_str());
    return;
  }
  auto field = structSym->fields->at(fieldName);
  node->attributes = field->attributes;
  if (node->children[1]->symbol == TOK_FIELD) {
    node->children[1]->attributes = field->attributes;
    node->children[1]->declLoc = field->lloc;
    node->children[1]->type = field->type;
  }
  if (node->attributes[unsigned(attr::STRUCT)]) {
    node->type = node->children[0]->type;
  }
  node->attributes[unsigned(attr::FIELD)] = false;
  node->attributes[unsigned(attr::VADDR)] = true;
}

void processCall(astree *node) {
  auto key = const_cast<string *>(node->children[0]->lexinfo);
  if (!globalDefs->count(key)) {
    errprintf("Function %s is undefined!\n", key->c_str());
    return;
  }

  auto sym = symbolFromNode(node, node->attributes);
  if (sym->parameters == nullptr) {
    sym->parameters = new vector<symbol *>;
  }
  for (size_t i = 1; i < node->children.size(); i++) {
    DEBUGF('d', "In call, attributes for node are:\n");
    printDebugAttribs(node->children[i]);
    sym->parameters->push_back(symbolFromNode(node->children[i],
                                              node->children[i]
                                                  ->attributes));
  }

  auto func = globalDefs->at(key);
  if (func->parameters->size() != sym->parameters->size()) {
    errprintf("Function %s does not match prototype!\n", key->c_str());
    return;
  }

  for (size_t i = 0; i < func->parameters->size(); i++) {
    if (!isTypeOkay(func->parameters->at(i), sym->parameters->at(i)
    )) {
      errprintf("Function %s does not match prototype in its "
                "parameters, pos %d!\n", key->c_str(), i + 1);
      DEBUGF('b', "Attributes from prototype are:\n");
      printDebugAttribsByAttr(func->parameters->at(i)->attributes);
      DEBUGF('b', "Attributes from this symbol are:\n");
      printDebugAttribsByAttr(sym->parameters->at(i)->attributes);
      return;
    }
  }

  node->type = func->type;
  sym->attributes = func->attributes;
  sym->attributes[unsigned(attr::FUNCTION)] = false;
  sym->attributes[unsigned(attr::VREG)] = true;
  node->attributes = sym->attributes;
  deleteSym(sym);
}

/*
|  '{' "{" 7.5.13
|  |  TOK_FIELD "string" 7.6.4
|  |  |  TOK_IDENT "data" 7.6.11
|  |  TOK_FIELD "node" 7.7.4
|  |  |  TOK_IDENT "link" 7.7.9
|  |  TOK_FIELD "[]" 7.8.7
|  |  |  TOK_INT "int" 7.8.4
|  |  |  TOK_IDENT "toot" 7.8.10
 */
void processStructFields(astree *node, symbol *sym) {
  for (auto &child : node->children) {
    symbol_entry ident = processIdent(attr::FIELD, child);
    if (child->children.size() > 1) {
      ident.second->attributes[unsigned(to_attr(child->children[0]
                                                    ->lexinfo))] = true;
      ident.second->attributes[unsigned(attr::ARRAY)] = true;
      DEBUGF('v', "Got attributes %s for struct field!\n",
             to_string(to_attr(child->children[0]->lexinfo)).c_str());
    } else {
      ident.second->attributes[unsigned(to_attr(child->lexinfo))] =
          true;
      DEBUGF('v', "Got attributes %s for struct field!\n",
             to_string(to_attr(child->lexinfo)).c_str());
    }
    ident.second->sequence = localSeq;
    if (sym->fields->count(ident.first)) {
      errprintf("Duplicate identifier specified in struct!\n");
      continue;
    }
    sym->fields->insert(ident);
    child->attributes = ident.second->attributes;
    if (ident.second->type != nullptr) {
      child->type = ident.second->type;
    } else {
      child->type = node->parent->lexinfo;
    }
    child->children[0]->attributes = ident.second->attributes;
    child->children[0]->type = ident.second->type;
    if (child->children.size() > 1) {
      child->children[1]->attributes = ident.second->attributes;
      child->children[1]->type = ident.second->type;
    }
    printToOutput(ident);
    localSeq++;
  }
}

/*
TOK_STRUCT "struct" 7.5.1
|  TOK_TYPEID "node" 7.5.8
|  '{' "{" 7.5.13
|  |  TOK_FIELD "string" 7.6.4
|  |  |  TOK_IDENT "data" 7.6.11
|  |  TOK_FIELD "node" 7.7.4
|  |  |  TOK_IDENT "link" 7.7.9
|  |  TOK_FIELD "[]" 7.8.7
|  |  |  TOK_INT "int" 7.8.4
|  |  |  TOK_IDENT "toot" 7.8.10
TOK_STRUCT "struct" 7.10.1
|  TOK_TYPEID "stack" 7.10.8
|  '{' "{" 7.10.14
|  |  TOK_FIELD "node" 7.11.4
|  |  |  TOK_IDENT "top" 7.11.9
*/
void processStruct(astree *node) {
  auto sym = symbolFromNode(node, node->attributes);
  sym->block_nr = 0;
  sym->type = const_cast<string *>(node->children[0]->lexinfo);

  node->attributes = sym->attributes;
  node->type = sym->type;

  // structure name must be inserted into the structure hash before
  // the field table is processed

  DEBUGF('v', "sym type is %s or %s\n",
         node->children[0]->lexinfo->c_str(),
         sym->type->c_str());
  auto pair = make_pair(sym->type, sym);
  structDefs->insert(pair);
  printToOutput(pair);
  sym->fields = new symbol_table;

  // empty struct defs
  localSeq = 0;
  if (node->children.size() > 1) {
    processStructFields(node->children[1], sym);
  }

  localSeq = 0;
}

int doesIdentExistInSymtables(astree *node) {
  auto identName = const_cast<string *>(node->lexinfo);
  auto globalFound = globalDefs->count(identName);
  auto localFound = localDefs != nullptr &&
      localDefs->count(identName);
  if (!globalFound && !localFound) {
    errprintf("Could not find identifier %s!\n", const_cast<const
    string *>(identName)->c_str());
    return 0;
  } else if (localFound) {
    return 1;
  } else {
    return 2;
  }
}

void typeCheckIdent(astree *node) {
  auto identLex = const_cast<string *>(node->lexinfo);
  if (localDefs != nullptr) {
    if (localDefs->count(identLex)) {
      node->attributes = localDefs->at(identLex)->attributes;
      node->type = localDefs->at(identLex)->type;
      node->declLoc = localDefs->at(identLex)->lloc;
    } else if (globalDefs->count(identLex)) {
      node->type = globalDefs->at(identLex)->type;
      node->attributes = globalDefs->at(identLex)->attributes;
      node->declLoc = globalDefs->at(identLex)->lloc;
    } else {
      DEBUGF('v', "Could not find ident %s definition!\n",
             identLex->c_str());
    }
  } else if (globalDefs->count(identLex)) {
    node->type = globalDefs->at(identLex)->type;
    node->attributes = globalDefs->at(identLex)->attributes;
    node->declLoc = globalDefs->at(identLex)->lloc;
  } else {
    DEBUGF('v', "Could not find ident %s definition!\n",
           identLex->c_str());
  }
}

symbol_entry processIdent(attr attrib, astree *node) {
  attr_bitset attributes = 0;
  auto autoattrib = unsigned(attrib);

  if (autoattrib) {
    attributes[autoattrib] = true;
  }

  if (attrib == attr::PARAM || attrib == attr::LVAL || attrib ==
      attr::VARIABLE) {
    attributes[unsigned(attr::LVAL)] = true;
    attributes[unsigned(attr::VARIABLE)] = true;
  }

  astree *ident;
  if (!node->children.empty()) {
    ident = node->children[0];
  } else {
    DEBUGF('v', "ident %s is node, please verify!\n",
           node->lexinfo->c_str());
    ident = node;
  }
  auto key = const_cast<string *>(ident->lexinfo);
  auto val = symbolFromNode(node, attributes);
  auto type = const_cast<string *>(node->lexinfo);
  auto isNodeArray = node->children.size() > 1;

  if (isNodeArray) {
    ident = node->children[1];
    key = const_cast<string *>(ident->lexinfo);
    type = const_cast<string *>(node->children[0]->lexinfo);
  }

  DEBUGF('v', "Got ident name %s\n", key->c_str());
  DEBUGF('v', "Structdef count of %s is %d\n", type->c_str(),
         structDefs->count(type));

  if (structDefs->count(type)) {
    DEBUGF('v', "Structdef count of %s was %d, setting struct to "
                "true!\n", type->c_str(), structDefs->count(type));
    val->type = type;
    attributes[unsigned(attr::STRUCT)] = true;
    auto typeattr = to_attr(type);
    DEBUGF('v', "node->children[0]->lexinfo is %s, type attribute is "
                "%s\n",
           node->children[0]->lexinfo->c_str(), to_string(typeattr)
               .c_str());

    attributes[(unsigned(to_attr(type)))] = true;
    node->type = type;
  } else {
    attributes[(unsigned(to_attr(type)))] = true;
  }

  if (isNodeArray) {
    attributes[unsigned(attr::ARRAY)] = true;
  }

  val->attributes = attributes;
  ident->block_nr = next_block;
  ident->attributes = val->attributes;
  symbol_entry returnVal = make_pair(key, val);
  node->attributes = attributes;

  printDebugAttribs(node);
  return returnVal;
}

/*
TOK_PARAM "(" 7.42.10
|  TOK_INT "int" 7.42.11
|  |  TOK_DECLID "argc" 7.42.15
|  TOK_ARRAY "[]" 7.42.27
|  |  TOK_STRING "string" 7.42.21
|  |  TOK_DECLID "argv" 7.42.30
*/
void processParams(symbol *func, astree *node) {
  if (func->parameters == nullptr) {
    func->parameters = new vector<symbol *>;
  }
  for (auto &child : node->children) {
    auto sym = processIdent(attr::PARAM, child);
    if (child->children.size() > 1) {
      child->attributes = child->children[1]->attributes;
    } else {
      child->attributes = child->children[0]->attributes;
    }
    child->declLoc = child->lloc;
    DEBUGF('b', "For child %s, attributes are:\n",
           child->lexinfo->c_str());
    printDebugAttribsByAttr(sym.second->attributes);
    DEBUGF('b', "For child of child %s, attributes are:\n",
           child->children[0]->lexinfo->c_str());
    printDebugAttribsByAttr(child->children[0]->attributes);
    child->type = sym.second->type;
    for (auto arrayChild : child->children) {
      arrayChild->type = child->type;
      arrayChild->declLoc = child->lloc;
    }
    sym.second->sequence = localSeq;
    localDefs->insert(sym);
    func->parameters->push_back(sym.second);
    printToOutput(sym);
    printDebugAttribs(child);
    localSeq++;
  }
}

/*
|  TOK_FUNCTION "" 7.68.5
|  |  TOK_ARRAY "[]" 7.68.5
|  |  |  TOK_VOID "void" 7.68.1
|  |  |  TOK_DECLID "f" 7.68.8
|  |  TOK_PARAM "(" 7.68.9
|  |  TOK_BLOCK "{" 7.68.12
|  TOK_PROTO "" 6.29.1
|  |  TOK_VOID "void" 6.29.1
|  |  |  TOK_DECLID "__assert_fail" 6.29.6
|  |  TOK_PARAM "(" 6.29.20
|  |  |  TOK_STRING "string" 6.29.21
|  |  |  |  TOK_DECLID "expr" 6.29.28
|  |  |  TOK_STRING "string" 6.29.34
|  |  |  |  TOK_DECLID "file" 6.29.41
|  |  |  TOK_INT "int" 6.29.47
|  |  |  |  TOK_DECLID "line" 6.29.51
|  |  |  TOK_STRING "string" 6.29.57
|  |  |  |  TOK_DECLID "func" 6.29.64
*/
void processFunction(astree *node) {
  string *name, *type;

  auto decls = node->children[0];

  if (decls->symbol == TOK_ARRAY) {
    type = const_cast<string *>(decls->children[0]->lexinfo);
    name = const_cast<string *>(decls->children[1]->lexinfo);
    if (decls->children[0]->symbol == TOK_VOID) {
      errprintf("Function %s cannot have return type of array "
                "void!\n", decls->children[1]->lexinfo->c_str());
      return;
    }
  } else {
    name = const_cast<string *>(decls->children[0]->lexinfo);
    type = const_cast<string *>(decls->lexinfo);
  }

  if (node->symbol != TOK_PROTO && globalDefs->count(name)) {
    auto sym = globalDefs->at(name);
    if (sym->attributes[unsigned(attr::FUNCTION)]) {
      errprintf("Duplicate declaration: Function %s has already been "
                "declared!\n", const_cast<const string *>(name)
                    ->c_str());
      return;
    }
  }

  DEBUGF('v', "Node symbol token == TOK_PROTO is %d\n", node->symbol ==
      TOK_PROTO);

  auto sym = symbolFromFunctionNode(node, node->attributes);

  sym->block_nr = 0;
  node->block_nr = next_block;

  if (structDefs->count(type)) {
    sym->attributes[unsigned(attr::STRUCT)] = true;
  } else {
    sym->attributes[unsigned(to_attr(type))] = true;
  }

  sym->type = type;
  node->type = type;

  if (node->symbol != TOK_PROTO) {
    sym->attributes[unsigned(attr::FUNCTION)] = true;
  }

  node->attributes = sym->attributes;
  printDebugAttribs(node);
  if (globalDefs->count(sym->type)) {
    node->attributes = globalDefs->at(sym->type)->attributes;
    node->attributes[unsigned(attr::FUNCTION)] = true;
  }

  node->block_nr = sym->block_nr;
  node->declLoc = node->lloc;

  auto entry = make_pair(name, sym);

  printToOutput(entry);

  localDefs = new symbol_table;
  localSeq = 0;
  DEBUGF('v', "Processing params of %s!\n",
         node->children[0]->children[(node->children[0]->symbol ==
             TOK_ARRAY) ? 1 : 0]->lexinfo->c_str());
  processParams(sym, node->children[1]);
  localSeq = 0;

  if (globalDefs->count(name)) {
    auto globalSym = globalDefs->at(name);
    // TODO(sahjain): PARAMETER TYPES
    if (globalSym->parameters->size() != sym->parameters->size()) {
      errprintf("Parameters of %s don't match prototype!\n",
                name->c_str());
      return;
    }
    for (uint i = 0; i < globalSym->parameters->size(); i++) {
      if (!isTypeOkay(globalSym->parameters->at(i),
                      sym->parameters->at(i))) {
        errprintf("Parameters of %s don't match prototype!\n",
                  const_cast<const string *>(name)->c_str());
        return;
      }
    }

    if (!isTypeOkay(globalSym, sym)) {
      errprintf("Function %s does not return same type as "
                "prototype!\n", name->c_str());
      return;
    }
  }

  if (node->children.size() > 2) {
    processBlock(node->children[2]);
  }

  globalDefs->insert(entry);
}

void processBlock(astree *node) {
  for (auto child : node->children) {
    scanAST(child);
  }
}

void typeCheckVarDecl(astree *node) {
  auto identTypeNode = node->children[0];
  auto equalNode = node->children[1];

  auto identNode = identTypeNode->children[0];
  if (identTypeNode->symbol == TOK_ARRAY) {
    identNode = identTypeNode->children[1];
  }

  auto symA = new symbol;
  symA->attributes = identTypeNode->attributes;
  symA->type = const_cast<string *>(identTypeNode->type);
  auto symB = new symbol;
  symB->attributes = equalNode->attributes;
  symB->type = const_cast<string *>(equalNode->type);

  DEBUGF('v', "SymA has attributes:\n");
  printDebugAttribsByAttr(symA->attributes);
  DEBUGF('v', "SymB has attributes:\n");
  printDebugAttribsByAttr(symB->attributes);

  if (!isTypeOkay(symA, symB)) {
    errprintf("Incompatible types for ident %s!\n",
              identNode->lexinfo->c_str());
  }

  deleteSym(symA);
  deleteSym(symB);
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
void processVarDecl(astree *node) {
  auto tableToInsert = (localDefs != nullptr) ? localDefs : globalDefs;

  string *type, *key;
  auto leftChild = node->children[0];
  auto rightChild = node->children[1];

  auto sym = symbolFromNode(node, node->attributes);
  setSymbolAttributes(rightChild, sym);

  if (localDefs != nullptr) {
    sym->sequence = localSeq;
    localSeq++;
  }

  if (leftChild->symbol == TOK_ARRAY) {
    type = const_cast<string *>
    (leftChild->children[0]->lexinfo);
    key = const_cast<string *>(node->children[0]->children[1]->lexinfo);
    sym->attributes[unsigned(attr::ARRAY)] = true;
  } else {
    type = const_cast<string *>(leftChild->lexinfo);
    key = const_cast<string *>(leftChild->children[0]->lexinfo);
  }

  if (localDefs != nullptr) {
    if (localDefs->count(key)) {
      errprintf("Duplicate declaration of %s!\n", key->c_str());
      return;
    }
  } else if (globalDefs->count(key)) {
    errprintf("Duplicate declaration of %s!\n", key->c_str());
    return;
  }

  sym->attributes[unsigned(attr::LVAL)] = true;
  sym->attributes[unsigned(attr::VARIABLE)] = true;

  if (localDefs != nullptr) {
    sym->attributes[unsigned(attr::LOCAL)] = true;
  }

  if (structDefs->count(type)) {
    sym->attributes[unsigned(attr::STRUCT)] = true;
    sym->type = type;
  } else {
    sym->attributes[unsigned(to_attr(type))] = true;
  }

  node->children[0]->attributes = sym->attributes;
  node->children[0]->type = sym->type;
  node->children[0]->declLoc = sym->lloc;

  for (auto child : node->children[0]->children) {
    child->attributes = sym->attributes;
    child->type = sym->type;
    child->declLoc = sym->lloc;
  }

  auto val = make_pair(key, sym);

  printToOutput(val);
  printDebugAttribs(node);
  if (node->children[0]->symbol != TOK_ARRAY) {
    printDebugAttribs(node->children[0]);
  } else {
    printDebugAttribs(node->children[1]);
  }

  tableToInsert->insert(val);
}

void printToOutput(symbol_entry sym) {
  bool useBraces = true, tabOver = false;

  attr_bitset bits = sym.second->attributes;

  if (bits[unsigned(attr::LOCAL)] || bits[unsigned(attr::PARAM)]) {
    useBraces = true;
    tabOver = true;
  } else if (bits[unsigned(attr::FIELD)]) {
    useBraces = false;
    tabOver = true;
  }

  if (tabOver) {
    fprintf(outputFile, "%*s", 3, "");
  }

  if (firstBracePlaced && !tabOver) {
    fprintf(outputFile, "\n");
  } else {
    firstBracePlaced = true;
  }

  fprintf(outputFile, "%s (%zd.%zd.%zd) ", sym.first->c_str(), sym
      .second->lloc
      .filenr, sym.second->lloc.linenr, sym.second->lloc.offset);

  if (useBraces) {
    fprintf(outputFile, "{%zd} ", sym.second->block_nr);
  }

  printAttribsToOutFile(outputFile, bits, sym.second->lloc, sym
                            .second->type,
                        false);

  if (bits[unsigned(attr::FIELD)] || bits[unsigned(attr::PARAM)] ||
      bits[unsigned(attr::LOCAL)]) {
    fprintf(outputFile, "%zd", sym.second->sequence);
  }
  fprintf(outputFile, "\n");
}

void printDebugAttribsByAttr(attr_bitset attrBitset) {
  static vector<attr> attrs{
      attr::VOID, attr::INT, attr::NULLX, attr::STRING, attr::STRUCT,
      attr::ARRAY, attr::FUNCTION, attr::VARIABLE, attr::LVAL,
      attr::FIELD, attr::TYPEID, attr::PARAM, attr::LOCAL, attr::CONST,
      attr::VREG, attr::VADDR,
  };

  for (auto child : attrs) {
    if (attrBitset[unsigned(child)]) {
      DEBUGF('d', "attr %s\n", to_string(child).c_str());
    }
  }
}

void printDebugAttribs(astree *node) {
  static vector<attr> attrs{
      attr::VOID, attr::INT, attr::NULLX, attr::STRING, attr::STRUCT,
      attr::ARRAY, attr::FUNCTION, attr::VARIABLE, attr::LVAL,
      attr::FIELD, attr::TYPEID, attr::PARAM, attr::LOCAL, attr::CONST,
      attr::VREG, attr::VADDR,
  };

  DEBUGF('v', "Node %s has attributes\n", node->lexinfo->c_str());
  for (auto child : attrs) {
    if (node->attributes[unsigned(child)]) {
      DEBUGF('v', "attr %s\n", to_string(child).c_str());
      if (child == attr::STRUCT) {
        if (node->type != nullptr) {
          DEBUGF('v', " type %s\n", node->type->c_str());
        } else {
          DEBUGF('v', " type nullptr\n");
        }
      } else if (child == attr::VARIABLE) {
        DEBUGF('v', "(%zd.%zd.%zd)\n", node->declLoc.filenr,
               node->declLoc.linenr, node->declLoc.offset);
      }
    }
  }
  DEBUGF('v', "!\n", node->lexinfo->c_str());
}

void printAttribsToOutFile(FILE *outFile,
                           attr_bitset attribs,
                           location declLoc,
                           const string *type,
                           bool typeQuotes) {
  static vector<attr> attrs{
      attr::VOID, attr::INT, attr::NULLX, attr::STRING, attr::STRUCT,
      attr::ARRAY, attr::FUNCTION, attr::VARIABLE, attr::LVAL,
      attr::FIELD, attr::TYPEID, attr::PARAM, attr::LOCAL,
      attr::CONST, attr::VREG, attr::VADDR
  };

  for (auto attrib : attrs) {
    if (attribs[unsigned(attrib)]) {
      DEBUGF('v', "In printAttribsToOutFile, node has attrib %s\n",
             to_string(attrib).c_str());
      fprintf(outFile, "%s ", to_string(attrib).c_str());
      if (attrib == attr::STRUCT) {
        if (typeQuotes) {
          fprintf(outFile, "\"%s\" ", type->c_str());
        } else {
          fprintf(outFile, "%s ", type->c_str());
        }
      } else if ((attrib == attr::FUNCTION || attrib == attr::VARIABLE)
          && typeQuotes) {
        fprintf(outFile, "(%zd.%zd.%zd) ", declLoc.filenr,
                declLoc.linenr, declLoc.offset);
      }
    }
  }
}

void deleteSym(symbol *sym) {
  if (sym != nullptr) {
    if (sym->fields != nullptr) {
      deleteSymTable(sym->fields);
    }
    if (sym->parameters != nullptr) {
      for (auto param : *sym->parameters) {
        deleteSym(param);
      }
      delete sym->parameters;
    }
  }
  delete sym;
}

void deleteSymTable(symbol_table *table) {
  if (table == nullptr) {
    return;
  }

  for (auto child : *table) {
    deleteSym(child.second);
    child.second = nullptr;
  }
  delete table;
}

void cleanupSymtables() {
  deleteSymTable(globalDefs);
  deleteSymTable(structDefs);
  deleteSymTable(localDefs);
}
