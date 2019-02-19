//
// Created by sahjain on 11/29/18.
//

#ifndef OILGENERATOR_H
#define OILGENERATOR_H

#include "lyutils.h"

enum NameType {
  NOTYPE,   // no mangling
  GLOBALNAME,
  LOCALNAME,
  VARNAME,
  STMTLABEL,
  STRUCTURETYPEIDS,
  STRUCTUREFIELDS,
  VIRTUALREGISTERS
};

void startOilGenerator(FILE *outFile);
string procStatement(astree* node);
#endif //OILGENERATOR_H
