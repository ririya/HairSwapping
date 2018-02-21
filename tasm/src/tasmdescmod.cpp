// tasmdescmod.cpp: generate the descriptor models
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.h"
#include "../tasm/src/tasm.h"

namespace stasm
{
typedef vector<vector<double> > TRAIN_DISTS; // [npoints] [ndesc] distances from true landmark
typedef vector<vector<VEC> >    TRAIN_DESCS; // [npoints] [ndesc] of descriptors

static void GetTrainImg(     // read image from disk, and scale it
    Image&           img,    // out: image scaled to EYEMOUTH_DIST and ilev
    Shape&           shape,  // out: shape scaled ditto
    int              ilev,   // in: pyramid level (0 is full scale)
    const ShapeFile& sh,     // in
    int              ishape) // in: shape index in the shapefile
{
    const char* imgpath = PathGivenDirs(sh.bases_[ishape], sh.dirs_, sh.shapepath_);
    Image fullimg(cv::imread(imgpath, CV_LOAD_IMAGE_GRAYSCALE));
    if (!fullimg.data)
        Err("Cannot load %s\n", imgpath);
    const double imgscale = EYEMOUTH_DIST / EyeMouthDist(sh.shapes_[ishape]);
    CV_Assert(imgscale > .1 && imgscale < 20); // arb limits
    // TODO add occasional noise to imgscale to simulate when can't find eyes in search?
    shape = sh.shapes_[ishape] * imgscale;
    cv::resize(fullimg, img,
               cv::Size(), imgscale, imgscale, cv::INTER_LINEAR);

#if 0 // TODO rotate image and shape so eyes are horizontal to match what stasm does
    Shape shape17(Shape17OrEmpty(shape, false)); // need eye angle
    if (shape17.rows) // converted shape to a Shape17 successfully?
    {
        // rotate image so eyes are horizontal
        const double eyeangle = EyeAngle(detpar_roi);
        if (Valid(eyeangle))
        {
            const double ROT_TREAT_AS_ZERO = 5; // small rots are treated as zero
            if (rot >= -ROT_TREAT_AS_ZERO && rot <= ROT_TREAT_AS_ZERO)
                rot = 0;
            if (rot)
            {
                warpAffine(img, img,
                           getRotationMatrix2D(cv::Point2f(float(detpar_roi.x),
                                                           float(detpar_roi.y)),
                                               -detpar.rot, 1.),
                           cv::Size(face_roi.cols, face_roi.rows),
                           cv::INTER_AREA, cv::BORDER_REPLICATE);
                rotate shape to match rotated image
            }
        }
    }
#endif
    const double lev_scale = GetPyrScale(ilev);
    cv::resize(img, img, cv::Size(), lev_scale, lev_scale, cv::INTER_LINEAR);
    shape *= lev_scale;
}

static void NegOffsets( // random x and y offset from the true landmark position
    int& xneg,          // out
    int& yneg)          // out
{
    CV_Assert(TASM_NEG_MIN_OFFSET > 0 && TASM_NEG_MIN_OFFSET < 100);
    CV_Assert(TASM_NEG_MAX_OFFSET > 0 && TASM_NEG_MAX_OFFSET < 100);
    CV_Assert(TASM_NEG_MAX_OFFSET >= TASM_NEG_MIN_OFFSET);

    xneg = rand() % (TASM_NEG_MAX_OFFSET - TASM_NEG_MIN_OFFSET + 1);
    xneg += TASM_NEG_MIN_OFFSET;

    if (rand() & 1)
        xneg = -xneg; // flip sign

    yneg = rand() % (TASM_NEG_MAX_OFFSET - TASM_NEG_MIN_OFFSET + 1);
    yneg += TASM_NEG_MIN_OFFSET;

    if (rand() & 1)
        yneg = -yneg;
}

// get the descriptors for all points in an image at one pyramid level

static void GenTrainingDescsForOneImg(
    TRAIN_DISTS&     train_dists, // io: [ipoint] [idesc] new distances will be added
    TRAIN_DESCS&     train_descs, // io: [ipoint] [idesc] new descriptors will be added
    int              ilev,        // in: pyramid level (0 is full scale)
    const ShapeFile& sh,          // in
    int              ishape)      // in: shape index in the shapefile
{
    Image img;   // image scaled to EYEMOUTH_DIST and ilev
    Shape shape; // shape scaled ditto

    GetTrainImg(img, shape, ilev, sh, ishape);

    // init internal HAT mats for this lev so can call HatDesc later
    InitHatLevData(img, ilev);

    for (int ipoint = 0; ipoint < shape.rows; ipoint++)
    {
        if (PointUsableForTraining(shape, ipoint,
                                   sh.bits_[ishape], sh.bases_[ishape].c_str()))
        {
            const bool is_hat = IsTasmHatDesc(ilev, ipoint);
            const VEC desc(
                is_hat? HatDesc(shape(ipoint, IX), shape(ipoint, IY)):
                        ClassicProf(img, shape, ipoint, TASM_1D_PROFLEN));

            train_dists[ipoint].push_back(0);    // positive case so distance is zero
            train_descs[ipoint].push_back(desc);
            if (is_hat) // get the negative descriptors too?
                for (int ineg = 0; ineg < TASM_NNEG_TRAIN_DESCS; ineg++)
                {
                    // get descriptor at at a random xy offset from the true position
                    int xneg, yneg; NegOffsets(xneg, yneg);
                    const VEC neg_desc(HatDesc(shape(ipoint, IX) + xneg,
                                               shape(ipoint, IY) + yneg));

                    train_dists[ipoint].push_back(sqrt(double(SQ(xneg)+SQ(yneg))));
                    train_descs[ipoint].push_back(neg_desc);
                }
        }
    }
}

static void CheckUnusedPoints(
    const TRAIN_DISTS& train_dists, // in: [ipoint] [idesc] distances from true landmark
    int                nshapes)     // in
{
    for (int ipoint = 0; ipoint < NSIZE(train_dists); ipoint++)
    {
        if (NSIZE(train_dists[ipoint]) == 0)
            Err("no descriptors were generated for point %d", ipoint);
        if (NSIZE(train_dists[ipoint]) < nshapes) // see PointUsableForTraining
        {
            static int printed;
            PrintOnce(printed,
                "\n    in point %d, only %d descriptors used from "
                "%d shapes because some points were skipped... ",
                ipoint, NSIZE(train_dists[ipoint]), nshapes);
        }
    }
}

static void GetTrainingDescs( // get the descriptors for all points and images for one pyr lev
    TRAIN_DISTS&     train_dists, // out: [ipoint] [idesc] distances from true landmark
    TRAIN_DESCS&     train_descs, // out: [ipoint] [idesc] training descriptors
    int              ilev,        // in: pyramid level (0 is full scale)
    const ShapeFile& sh)          // in
{
    clock_t start_time = clock();
    lprintf("Pyramid level %d reading %d training images ", ilev, sh.nshapes_);
    logprintf("\n");
    const int npoints = sh.shapes_[0].rows;
    train_dists.resize(npoints);
    train_descs.resize(npoints);
    Pacifier pacifier(sh.nshapes_);
    for (int ishape = 0; ishape < sh.nshapes_; ishape++)
    {
        GenTrainingDescsForOneImg(train_dists, train_descs, ilev, sh, ishape);
        CheckMem(); // warn if are about to run out of memory
        if (print_g)
            pacifier.Print_(ishape);
    }
    if (print_g) // no tasm -q flag?
    {
        pacifier.End_();
        printf(" ");
    }
    CheckUnusedPoints(train_dists, sh.nshapes_);
    lprintf("[%.1f secs]\n", double(clock() - start_time) / CLOCKS_PER_SEC);
}

static void WriteTrainDescs(
    const TRAIN_DISTS& train_dists, // in: [ipoint] [idesc] distances from true landmark
    const TRAIN_DESCS& train_descs, // in: [ipoint] [idesc] training descriptors
    int                ilev,        // in:
    const char*        outdir)      // in: output directory (-d flag)
{
    clock_t start_time = clock();
    const int npoints = NSIZE(train_descs);
    Pacifier pacifier(npoints);
    for (int ipoint = 0; ipoint < npoints; ipoint++)
    {
        // filename
        const int ndigits = NumDigits(npoints);
        char format[SLEN]; // e.g. tasmout/log/lev1_p23_classic.desc
        sprintf(format, "%%s/log/lev%%d_p%%%d.%dd_%%s.desc", ndigits, ndigits);
        char path[SLEN];
        sprintf(path, format, outdir, ilev, ipoint,
                IsTasmHatDesc(ilev, ipoint)? "hat": "classic");
        static int printed;
        PrintOnce(printed, "    generating %s (tasm -w flag)...\n", path);
        FILE* file = fopen(path, "wb");
        if (!file)
            Err("Cannot open %s", path);
        // header
        const vec_VEC descs = train_descs[ipoint]; // descriptors for this point
        const vec_double dists = train_dists[ipoint];
        Fprintf(file, "ilev ipoint       dist ");
        for (int i = 0; i < NSIZE(descs[0]); i++)
        {
            char s[SLEN];  sprintf(s, "x%d ", i);
            Fprintf(file, " %10.10s", s);
        }
        Fprintf(file, "\n");
        // contents
        for (int idesc = 0; idesc < NSIZE(descs); idesc++)
        {
            Fprintf(file, "%4d %6d %10g", ilev, ipoint, dists[idesc]);
            for (int i = 0; i < NSIZE(descs[idesc]); i++)
                Fprintf(file, " %10g", descs[idesc](i));
            Fprintf(file, "\n");
        }
        fclose(file);
        if (print_g)
            pacifier.Print_(ipoint);
    }
    if (print_g)
        pacifier.End_();
    lprintf(" [%.1f secs]\n", double(clock() - start_time) / CLOCKS_PER_SEC);
}

static void GenDescMods( // generate descriptor models for all points and images for one pyr lev
    TRAIN_DISTS&     train_dists, // out: [ipoint] [idesc] distances from true landmark
    TRAIN_DESCS&     train_descs, // out: [ipoint] [idesc] training descriptors
    int              ilev,        // in: pyramid level (0 is full scale)
    const ShapeFile& sh,          // in
    const char*      outdir,      // in: output directory (-d flag)
    const char*      modname,     // in: e.g. "yaw00"
    const char*      cmdline)     // in: command line used to invoke tasm
{
    clock_t start_time = clock();
    lprintf("Generating pyramid level %d models ", ilev);
    const int npoints = sh.shapes_[0].rows;
    Pacifier pacifier(npoints);
    for (int ipoint = 0; ipoint < npoints; ipoint++)
    {
        const vec_VEC descs = train_descs[ipoint]; // descriptors for this point
        CV_Assert(NSIZE(descs));
        char msg[SLEN]; sprintf(msg, "(level %d point %d)", ilev, ipoint);
        if (IsTasmHatDesc(ilev, ipoint)) // HAT descriptor?
            GenHatMod(ilev, ipoint, npoints, train_dists[ipoint], descs,
                      msg, outdir, modname, cmdline);
        else // classical 1D profile
            GenClassicMod(ilev, ipoint, npoints, descs, msg, outdir, modname, cmdline);
        if (print_g)
            pacifier.Print_(ipoint);
    }
    if (print_g)
        pacifier.End_();
    lprintf(" [%.1f secs]\n", double(clock() - start_time) / CLOCKS_PER_SEC);
}

void GenAndWriteDescMods( // generate the descriptor models and write them to .mh files
    const ShapeFile& sh,               // in: the shapefile contents
    const char*      outdir,           // in: output directory (-d flag)
    const char*      modname,          // in: e.g. "yaw00"
    const char*      cmdline,          // in: command line used to invoke tasm
    bool             write_traindescs) // in: tasm -w flag
{
    lprintf("--- Generating the descriptor models from %d shapes ---\n", NSIZE(sh.shapes_));

    srand((unsigned int)TASM_NEGTRAIN_SEED);

    // we work a level at a time to save memory (although it takes longer than
    // doing all levels simultaneously because we have to reread the images for
    // each level)

    for (int ilev = 0; ilev < N_PYR_LEVS; ilev++)
    {
        TRAIN_DISTS train_dists; // [ipoint] [idesc] distances from true landmark
        TRAIN_DESCS train_descs; // [ipoint] [idesc] training descriptors

        // get the training descriptors for all points and images for this pyr lev

        GetTrainingDescs(train_dists, train_descs, ilev, sh);

        if (write_traindescs)
            WriteTrainDescs(train_dists, train_descs, ilev, outdir);

        // generate the descriptor models from the training descriptors

        GenDescMods(train_dists, train_descs, ilev, sh, outdir, modname, cmdline);
    }
}

} // namespace stasm
