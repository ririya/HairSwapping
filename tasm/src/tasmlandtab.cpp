// tasmlandtab.cpp: utilities that use the landmark table, generate model file e.g. yaw00.mh
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
void SanityCheckLandTab(void)
{
    const int npoints = NELEMS(LANDMARK_INFO_TAB);
    int npartners = 0; // number of points that have partners
    vec_int npartners_tab(npoints, 0);

    int ipoint;
    for (ipoint = 0; ipoint < npoints; ipoint++)
    {
        const LANDMARK_INFO info = LANDMARK_INFO_TAB[ipoint];

        if (info.prev != -1 &&
           (info.prev == ipoint || info.prev < 0 || info.prev >= npoints))
            Err("landmark table (%d points) test A: point %d has a bad prev %d",
                 npoints, ipoint, info.prev);

        if (info.next != -1 &&
           (info.next == ipoint || info.next < 0 || info.next >= npoints))
            Err("landmark table (%d points) test B: point %d has a bad next %d",
                 npoints, ipoint, info.next);

        // if (info.next >= 0 && info.prev < 0)
        //     Err("landmark table (%d points) test C: point %d has a next but not a prev",
        //          npoints, ipoint);

        // if (info.next < 0 && info.prev >= 0)
        //     Err("landmark table (%d points) test D: point %d has a prev but not a next",
        //          npoints, ipoint);

        if (info.weight < 0 || info.weight > 1)
            Err("landmark table (%d points) test E: point %d has a bad weight %g",
                 npoints, ipoint, info.weight);

        if (info.partner != -1)
        {
            npartners++;
            npartners_tab[info.partner]++;

            if (info.partner == ipoint || info.partner < 0 || info.partner >= npoints)
                Err("landmark table (%d points) test F: point %d has a bad partner %d",
                     npoints, ipoint, info.partner);

            // check that my partner's partner is me
            if (LANDMARK_INFO_TAB[info.partner].partner != ipoint)
                Err("landmark table (%d points) test F: points %d and %d "
                    "are partners but are not symmetrical\n"
                    "(point %d partner is %d but point %d partner is %d)",
                    npoints, ipoint, info.partner,
                    ipoint, info.partner,
                    info.partner, LANDMARK_INFO_TAB[info.partner].partner);

            // if I have a previous point then my partner should also have a previous point
            if ((info.prev <  0 && LANDMARK_INFO_TAB[info.partner].prev >= 0) ||
               (info.prev >= 0 && LANDMARK_INFO_TAB[info.partner].prev < 0))
                Err("landmark table (%d points) test G: points %d and %d "
                    "are partners but are not symmetrical\n"
                    "(point %d has a prev point but point %d does not have a prev point)",
                    npoints, ipoint, info.partner,
                    ipoint, info.partner);

            // if I have a next point then my partner should also have a next point
            if ((info.next <  0 && LANDMARK_INFO_TAB[info.partner].next >= 0) ||
               (info.next >= 0 && LANDMARK_INFO_TAB[info.partner].next < 0))
                Err("landmark table test H (%d points): points %d and %d "
                    "are partners but are not symmetrical\n",
                    "(point %d has a next point but point %d does not have a next point)",
                    npoints, ipoint, info.partner,
                    ipoint, info.partner);
        }
    }
    for (ipoint = 0; ipoint < npoints; ipoint++)
        if (npartners_tab[ipoint] > 1)
            Err("landmark table test J (%d points): "
                "point %d is the partner of more than one point",
                npoints, ipoint);

    if (npartners == 0)
        lprintf("No points have partners in the %d point landmark table\n", npoints);
}

void GenIncludeFileFromLandTab( // generate yaw00.h from LANDMARK_INFO_TAB
    const char* outdir,         // in: output directory (-d flag)
    const char* modname,        // in: e.g. "yaw00"
    const char* cmdline)        // in: command line used to invoke tasm
{

    lprintf("--- Generating %s.mh from the landmark table ---\n", TASM_MODNAME);
    char path[SLEN];
    sprintf(path, "%s/mh/%s.mh", outdir, modname);
    lprintf("Generating %s\n", path);
    FILE* file = fopen(path, "wb");
    if (!file)
        Err("Cannot open %s", path);
    Fprintf(file, "// %s.mh: list of ASM descriptor models (machine generated "
                  "from a %d point landmark table)\n",
                   modname, NELEMS(LANDMARK_INFO_TAB));
    Fprintf(file, "//\n");
    Fprintf(file, "// Command: %s\n", cmdline);
    Fprintf(file, "// STASM_VERSION \"%s\"\n", STASM_VERSION);
    Fprintf(file, "// TASM_MODNAME \"%s\"\n", TASM_MODNAME);
    Fprintf(file, "// TASM_DATADIR \"%s\"\n", TASM_DATADIR);
    Fprintf(file, "// TASM_DISALLOWED_ATTR_BITS 0x%x\n", TASM_DISALLOWED_ATTR_BITS);
    Fprintf(file, "// TASM_MEANSHAPE_USES_SHAPES_WITH_MISSING_POINTS %d\n",
                  TASM_MEANSHAPE_USES_SHAPES_WITH_MISSING_POINTS);
    Fprintf(file, "// TASM_REBUILD_SUBSET %s\n",
                  TASM_REBUILD_SUBSET? TASM_REBUILD_SUBSET: "NULL");
    Fprintf(file, "// TASM_SUBSET_DESC_MODS %d\n", TASM_SUBSET_DESC_MODS);
    Fprintf(file, "// TASM_FACEDET_MINWIDTH %d\n", TASM_FACEDET_MINWIDTH);
    Fprintf(file, "// TASM_DET_EYES_AND_MOUTH %d\n", TASM_DET_EYES_AND_MOUTH);
    Fprintf(file, "// TASM_1D_PROFLEN %d\n", TASM_1D_PROFLEN);
    Fprintf(file, "// TASM_NNEG_TRAIN_DESCS %d\n", TASM_NNEG_TRAIN_DESCS);
    Fprintf(file, "// TASM_NEG_MIN_OFFSET %d\n", TASM_NEG_MIN_OFFSET);
    Fprintf(file, "// TASM_NEG_MAX_OFFSET %d\n", TASM_NEG_MAX_OFFSET);
    Fprintf(file, "// TASM_NEGTRAIN_SEED %d\n", TASM_NEGTRAIN_SEED);
    Fprintf(file, "// TASM_BALANCE_NUMBER_OF_CASES %d\n", TASM_BALANCE_NUMBER_OF_CASES);
    Fprintf(file, "// TASM_BINARY_DISTANCE %d\n", TASM_BINARY_DISTANCE);
    Fprintf(file, "// HAT_PATCH_WIDTH %d\n", HAT_PATCH_WIDTH);
    Fprintf(file, "// HAT_PATCH_WIDTH_ADJ %d\n", HAT_PATCH_WIDTH_ADJ);
    Fprintf(file, "// HAT_START_LEV %d\n", HAT_START_LEV);
    Fprintf(file, "// EYEMOUTH_DIST %d\n", EYEMOUTH_DIST);
    Fprintf(file, "// PYR_RATIO  %d\n", PYR_RATIO );
    Fprintf(file, "// N_PYR_LEVS %d\n", N_PYR_LEVS);
    Fprintf(file, "\n");
    char path1[SLEN];
    sprintf(path1, "stasm_%s_mh", modname);
    ToUpper(path1);
    Fprintf(file, "#ifndef %s\n", path1);
    Fprintf(file, "#define %s\n", path1);
    Fprintf(file, "\n");
    Fprintf(file, "#include \"mh/yaw00_shapemodel.mh\"\n");
    Fprintf(file, "\n");
    const int npoints = NELEMS(LANDMARK_INFO_TAB);
    char format[SLEN]; // e.g. #include "mh/%s_lev%d_p%2.2d_%s.mh"\n
    const int ndigits = NumDigits(npoints);
    sprintf(format, "#include \"mh/%%s_lev%%d_p%%%d.%dd_%%s.mh\"\n", ndigits, ndigits);

    for (int ilev = 0; ilev < N_PYR_LEVS; ilev++)
        for (int ipoint = 0; ipoint < npoints; ipoint++)
            Fprintf(file, format,
                modname, ilev, ipoint,
                (LANDMARK_INFO_TAB[ipoint].bits & AT_Hat) && ilev <= HAT_START_LEV?
                    "hat": "classic");

    Fprintf(file, "\n");
    Fprintf(file, "namespace stasm\n");
    Fprintf(file, "{\n");
    char modupper[SLEN]; sprintf(modupper, modname); ToUpper(modupper);
    Fprintf(file, "// %s_DESCMODS defines the descriptor model to use at each point\n", modupper);
    Fprintf(file, "static const BaseDescMod* %s_DESCMODS[] =\n", modupper);
    Fprintf(file, "{\n");
    sprintf(format, "    &%%s_lev%%d_p%%%d.%dd_%%s%%s\n", ndigits, ndigits); // e.g. &%s_lev%d_p%2.2d_%s%s\n

    for (int ilev = 0; ilev < N_PYR_LEVS; ilev++)
        for (int ipoint = 0; ipoint < npoints; ipoint++)
            Fprintf(file, format,
                modname, ilev, ipoint,
                (LANDMARK_INFO_TAB[ipoint].bits & AT_Hat) && ilev <= HAT_START_LEV?
                    "hat": "classic",
                ilev == N_PYR_LEVS-1 && ipoint == npoints-1? "": ",");

    Fprintf(file, "};\n");
    Fprintf(file, "\n");
    Fprintf(file, "} // namespace stasm\n");
    Fprintf(file, "#endif // %s\n", path1);
    fclose(file);
}

} // namespace stasm
