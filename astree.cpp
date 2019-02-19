// $Id: astree.cpp,v 1.9 2017-10-04 15:59:50-07 - - $

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "astree.h"
#include "string_set.h"
#include "lyutils.h"
#include "symboltable.h"

astree::astree(int symbol_, const location &lloc_, const char *info) {
  symbol = symbol_;
  lloc = lloc_;
  lexinfo = string_set::intern(info);
  // vector defaults to empty -- no children
}

astree::~astree() {
  while (not children.empty()) {
    astree *child = children.back();
    children.pop_back();
    delete child;
  }

  if (yydebug) {
    fprintf(stderr, "Deleting astree (");
    astree::dump(stderr, this);
    fprintf(stderr, ")\n");
  }
}

astree *astree::adopt(astree *child1, astree *child2, astree *child3) {
  if (child1 != nullptr) {
    children.push_back(child1);
  }
  if (child2 != nullptr) {
    children.push_back(child2);
  }
  if (child3 != nullptr) {
    children.push_back(child3);
  }
  return this;
}

/**
 * Adopts child after changing current tree's symbol to symbol.
 *
 * @param child1    First child to adopt.
 * @param symbol_   Symbol to change this to.
 * @param child2    Second child to adopt.
 * @return          Child to adopt.
 */
astree *astree::adopt_sym(astree *child1, int symbol_,
                          astree *child2) {
  symbol = symbol_;
  return adopt(child1, child2);
}

void astree::dump_node(FILE *outfile) {
  fprintf(outfile, "%p->{%s %zd.%zd.%zd \"%s\":",
          this, parser::get_tname(symbol),
          lloc.filenr, lloc.linenr, lloc.offset,
          lexinfo->c_str());
  for (auto &child : children) {
    fprintf(outfile, " %p", child);
  }
}

void astree::dump_tree(FILE *outfile, int depth) {
  fprintf(outfile, "%*s", depth * 3, "");
  dump_node(outfile);
  fprintf(outfile, "\n");
  for (astree *child: children) child->dump_tree(outfile, depth + 1);
  fflush(nullptr);
}

void astree::dump(FILE *outfile, astree *tree) {
  if (tree == nullptr) fprintf(outfile, "nullptr");
  else tree->dump_node(outfile);
}

void astree::print(FILE *outfile, astree *tree, int depth) {
  for (int i = 1; i <= depth; i++) {
    fprintf(outfile, "|%*s", 2, "");
  }
  fprintf(outfile, "%s \"%s\" (%zd.%zd.%zd) {%zd} ",
          parser::get_tname(tree->symbol), tree->lexinfo->c_str(),
          tree->lloc.filenr, tree->lloc.linenr, tree->lloc.offset,
          tree->block_nr);
  printAttribsToOutFile(outfile, tree->attributes, tree->declLoc,
                        tree->type, true);
  fprintf(outfile, "\n");
  for (auto child: tree->children) {
    printDebugAttribs(child);
    astree::print(outfile, child, depth + 1);
  }
}

void destroy(astree *tree1, astree *tree2, astree *tree3) {
  delete tree1;
  delete tree2;
  delete tree3;
}

void errllocprintf(const location &lloc, const char *format,
                   const char *arg) {
  static char buffer[0x1000];
  assert (sizeof buffer > strlen(format) + strlen(arg));
  snprintf(buffer, sizeof buffer, format, arg);
  errprintf("%s:%zd.%zd: %s",
            lexer::filename(lloc.filenr)->c_str(), lloc.linenr,
            lloc.offset, buffer);
}
