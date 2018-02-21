// tasm.h: include files for tasm
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef stasm_tasm_hpp
#define stasm_tasm_hpp

#include "opencv/highgui.h" // needed for imread
#if _MSC_VER // microsoft
#include <direct.h>
#endif
#include "../../apps/appmisc.h"
#include "../../apps/appmem.h"
#include "../../apps/linsolve.h"
#include "../../apps/shapefile/shapefile.h"
#include "../../apps/shapefile/stasm_regex.h"
#include "../tasm/src/tasmconf.h"
#include "../tasm/src/tasmdescmod.h"
#include "../tasm/src/tasmdraw.h"
#include "../tasm/src/tasmlandtab.h"
#include "../tasm/src/tasmhat.h"
#include "../tasm/src/tasmclassic.h"
#include "../tasm/src/tasmmeanshape.h"
#include "../tasm/src/tasmshapemod.h"
#include "../tasm/src/tasmalignshapes.h"
#include "../tasm/src/tasmimpute.h"

#endif // stasm_tasm_hpp
