// tasmlandtab.h: utilities that use the landmark table, generate model file e.g. yaw00.mh
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef STASM_TASMLANDTAB_HPP
#define STASM_TASMLANDTAB_HPP

namespace stasm
{
void SanityCheckLandTab(void);

void GenIncludeFileFromLandTab( // generate yaw00.h from LANDMARK_INFO_TAB
    const char*      outdir,    // in: output directory (-d flag)
    const char*      modname,   // in: e.g. "yaw00"
    const char*      cmdline);  // in: command line used to invoke tasm

} // namespace stasm
#endif // STASM_TASMLANDTAB_HPP
