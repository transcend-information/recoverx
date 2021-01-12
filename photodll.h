#ifndef PHOTODLL_H
#define PHOTODLL_H

#include "photodll_global.h"

class PHOTODLLSHARED_EXPORT Photodll
{

public:
    Photodll();

    int startRevocer(char path[], char condition[],char outputpath[]);
};


#endif // PHOTODLL_H
