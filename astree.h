// $Id: astree.h,v 1.7 2016-10-06 16:13:39-07 - - $

#ifndef __ASTREE_H__
#define __ASTREE_H__

#include <string>
#include <vector>
#include <bitset>
#include <unordered_map>
using namespace std;

#include "auxlib.h"

struct location {
  size_t filenr{0};
  size_t linenr{0};
  size_t offset{0};
};

enum class attr {
  VOID, INT, NULLX, STRING, STRUCT, ARRAY, FUNCTION, VARIABLE, FIELD,
  TYPEID, PARAM, LOCAL, LVAL, CONST, VREG, VADDR, BITSET_SIZE,
};

struct symbol;

using attr_bitset = bitset<unsigned(attr::BITSET_SIZE)>;
using symbol_table = unordered_map<string *, symbol *>;
using symbol_entry = symbol_table::value_type;

struct symbol {
  string *type{};
  attr_bitset attributes;
  size_t sequence{};
  symbol_table *fields{};
  location lloc;
  size_t block_nr{};
  vector<symbol *> *parameters{};
};

struct astree {

  // Fields.
  int symbol;               // token code
  location lloc;            // source location
  const string *lexinfo;    // pointer to lexical information
  vector<astree *> children; // children of this n-way node
  astree *parent;
  string assignedReg;       // assigned register of node

  const string *type{};
  location declLoc;

  attr_bitset attributes{};
  size_t block_nr;
  symbol_entry *structTableNode;

  // Functions.
  astree(int symbol, const location &, const char *lexinfo);
  ~astree();
  astree *adopt(astree *child1, astree *child2 = nullptr,
                astree *child3 = nullptr);
  astree *adopt_sym(astree *child1, int symbol,
                    astree *child2 = nullptr);
  void dump_node(FILE *);
  void dump_tree(FILE *, int depth = 0);
  static void dump(FILE *outfile, astree *tree);
  static void print(FILE *outfile, astree *tree, int depth = 0);
};

void destroy(astree *tree1, astree *tree2 = nullptr,
             astree *tree3 = nullptr);

void errllocprintf(const location &, const char *format, const char *);

#endif

