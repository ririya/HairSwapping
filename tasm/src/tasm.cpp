// tasm.cpp: main routine for tasm executable (for building models)
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
// Sanity check the tasm params. This doesn't check everything.
// Many of the limits below are arbitrary.

static void TasmSanityChecks(
    int         npoints,      // in: number of points in shapes in shapefile
    const char* shapepath)    // shapefile name
{
    CV_Assert(EYEMOUTH_DIST > 10 && EYEMOUTH_DIST < 500);

    CV_Assert(PYR_RATIO > 0 && PYR_RATIO < 5);

    CV_Assert(N_PYR_LEVS > 0 && N_PYR_LEVS < 10);

    CV_Assert(TASM_MODNAME[0]);

    CV_Assert(TASM_NEGTRAIN_SEED > 0 && TASM_NEGTRAIN_SEED < 1e6);

    CV_Assert(TASM_NNEG_TRAIN_DESCS > 0 && TASM_NNEG_TRAIN_DESCS < 100);

    CV_Assert(TASM_NEG_MIN_OFFSET > 0 && TASM_NEG_MIN_OFFSET < 100);

    CV_Assert(TASM_NEG_MAX_OFFSET > 0 && TASM_NEG_MAX_OFFSET < 100);

    CV_Assert(TASM_NEG_MAX_OFFSET >= TASM_NEG_MIN_OFFSET);

    CV_Assert(TASM_BALANCE_NUMBER_OF_CASES == true ||
              TASM_BALANCE_NUMBER_OF_CASES == false);

    CV_Assert(TASM_BINARY_DISTANCE == true ||
              TASM_BINARY_DISTANCE == false);

    CV_Assert(HAT_START_LEV >= 0 && HAT_START_LEV <= N_PYR_LEVS);

    if (stasm_NLANDMARKS != npoints)
    {
        // It's safest to fix this problem and rebuild stasm and tasm.  But this is
        // a lprintf_always instead of Err to allow tasm to run, to avoid a catch-22
        // situation where we can't create a new model with tasm but don't want the
        // old model.
        lprintf_always(
            "Warning: stasm_NLANDMARKS %d does not match "
            "the number of points %d in %s\n",
            stasm_NLANDMARKS, npoints, BaseExt(shapepath));
    }
    if (NELEMS(LANDMARK_INFO_TAB) != npoints)
        Err("NELEMS(LANDMARK_INFO_TAB) %d "
            "does not match the number of points %d in %s",
            NELEMS(LANDMARK_INFO_TAB), npoints, BaseExt(shapepath));
}

static void CreateOutputDirs(
    const char* outdir)       // in
{
    char mh[SLEN];  sprintf(mh,  "%s/mh", outdir);
    char log[SLEN]; sprintf(log, "%s/log", outdir);
    // Remove old output files if any.
    // The test against '/' is just paranoia to prevent mistaken rmdirs.
    CV_Assert(outdir && outdir[1] && outdir[1] != '/' && outdir[1] != '\\');
    if (DirWriteable(log, false))
        System(ssprintf("rm -f %s/*", log));
    if (DirWriteable(mh, false))
        System(ssprintf("rm -f %s/*", mh));
    MkDir(outdir);
    MkDir(log);
    MkDir(mh);
}

static void main1(
    int          argc,
    const char** argv)
{
    static const char* const usage =
"tasm [FLAGS] file.shape [N [SEED [REGEX]]]\n"
"\n"
"Examples:\n"
"    tasm file.shape                 (all faces in file.shape)\n"
"    tasm file.shape 300             (first 300 faces in file.shape)\n"
"    tasm file.shape 300 99          (300 random faces, random seed is 99)\n"
"    tasm file.shape 0 0 x           (all faces with \"x\" in their name)\n"
"    tasm file.shape 300 99 \"xy|z\"   "
"(300 random faces with \"xy\"  or \"z\" in their name)\n"
"\n"
"Flags:\n"
"    -d DIR  output directory, default tasmout\n"
"    -f      face detector on images themselves (ignore shapefile facedets)\n"
"    -i      no output images, no facedet.shape, no imputed.shape (faster)\n"
"    -q      quiet (no prints except for warnings and errors)\n"
"    -w      write training descriptors to .desc files for post-processing\n"
"    -?      help\n"
"\n"
"tasm version %s    www.milbo.users.sonic.net/stasm\n"; // %s is stasm_VERSION

    clock_t start_time = clock();
    bool force_facedet = false;
    bool writeimgs = true; // -i flag
    bool write_traindescs = false; // -w flag
    print_g = true; // want to be able to see lprintfs
    char outdir[SBIG]; strcpy(outdir, "tasmout");
    char cmdline[SBIG]; CmdLineAsSingleString(cmdline, argc, argv);
    if (argc < 2)
        Err("No shapefile argument.  Use tasm -? for help.");
    while (--argc > 0 && (*++argv)[0] == '-')
    {
        if ((*argv + 1)[1])
            Err("Invalid argument -%s (there must be a space after -%c).  "
                "Use tasm -? for help.",
                *argv + 1, (*argv + 1)[0]);
        switch ((*argv + 1)[0])
        {
        case 'd':               // -d
            argv++;
            argc--;
            if (argc < 2)
                Err("Not enough command line arguments.  Use tasm -? for help.");
            strncpy_(outdir, *argv, 200); // 200 < SLEN, leave room for "/classic"
            ConvertBackslashesToForwardAndStripFinalSlash(outdir);
            if (STRNLEN(outdir, 200) < 1)
                Err("Invalid argument -d.  Use tasm -? for help.");
            break;
        case 'f':               // -f
            force_facedet = true;
            break;
        case 'i':               // -i
            writeimgs = false;
            break;
        case 'q':               // -q
            print_g = false;
            break;
        case 'w':               // -w
            write_traindescs = true;
            break;
        case '?':               // -?
            printf(usage, stasm_VERSION);
            exit(1);
        default:
            Err("Invalid argument -%s.  Use tasm -? for help.", *argv + 1);
            break;
        }
    }
    CreateOutputDirs(outdir);
    // open log file and print log file header
    OpenLogFile(ssprintf("%s/log/tasm.log", outdir));
    logprintf("tasm version %s\n", STASM_VERSION);
    logprintf("Command: %s\n", cmdline);
    // argv is now file.shape
    ShapeFile sh;
    ProcessShapeFileArg(sh, NULL, argc, argv, TASM_DISALLOWED_ATTR_BITS, 0);
    // done processing command line arguments
    CheckAllShapesHaveSameNumberOfLandmarks(sh);
    TasmSanityChecks(sh.shapes_[0].rows, sh.shapepath_);
    GenAndWriteShapeMod(sh, outdir, TASM_MODNAME, cmdline,
                        force_facedet, writeimgs, writeimgs, writeimgs);
    SanityCheckLandTab();
    GenIncludeFileFromLandTab(outdir, TASM_MODNAME, cmdline);
    GenAndWriteDescMods(sh, outdir, TASM_MODNAME, cmdline, write_traindescs);
    const double totaltime = double(clock() - start_time) / CLOCKS_PER_SEC;
    lprintf("[Total time %.1f secs%s, %d%% physical mem all processes, "
            "%.0fMB peak this process]\n",
            totaltime,
            totaltime > 180? ssprintf(" (%.1f minutes)", totaltime / 60): "",
            PercentPhysicalMemory(), double(PeakMemThisProcess()) / 1e6);
}

} // namespace stasm

// This application calls Stasm's internal routines.  Thus we need to catch a
// potential throw from Stasm's error handlers.  Hence the try/catch code below.

int main(int argc, const char** argv)
{
    stasm::CatchOpenCvErrs();
    try
    {
        stasm::main1(argc, argv);
    }
    catch(...)
    {
        // a call was made to Err or a CV_Assert failed
        printf("\n%s\n", stasm_lasterr());
        exit(1);
    }
    return 0; // success
}
