#include "co.h"
#include <stdio.h>
#include <string.h>

int main() {
  co xml = coReadXMLByFP(stdin, 1);
  coPrint(xml);
  coDelete(xml);
  return 0;
}

