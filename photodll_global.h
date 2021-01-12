#ifndef PHOTODLL_GLOBAL_H
#define PHOTODLL_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(PHOTODLL_LIBRARY)
#  define PHOTODLLSHARED_EXPORT Q_DECL_EXPORT
#else
#  define PHOTODLLSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // PHOTODLL_GLOBAL_H