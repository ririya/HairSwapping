// tasmdescmod.h: generate the descriptor models
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef stasm_tasmdescmod_hpp
#define stasm_tasmdescmod_hpp

namespace stasm
{
void GenAndWriteDescMods( // generate the descriptor models and write them to .mh files
    const ShapeFile& sh,                // in: the shapefile contents
    const char*      outdir,            // in: output directory (-d flag)
    const char*      modname,           // in: e.g. "yaw00"
    const char*      cmdline,           // in: command line used to invoke tasm
    bool             write_traindescs); // in: tasm -w flag

} // namespace stasm
#endif // stasm_tasmdescmod_hpp
