// tasmconf.h: configuration constants for tasm
//
// The values of the constants below seem optimal for building 77 point
// frontal models with the MUCT77 set and using linear models for
// descriptor matching.  Your mileage may vary.
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef stasm_tasmconf_hpp
#define stasm_tasmconf_hpp

namespace stasm
{
// The model name is used when generating the output filenames etc.
// It allows us to generate multiple submodels for multiple-view versions of Stasm.

static const char* const TASM_MODNAME = "yaw00";

// The data directory holds the face and feature detector XML files.

static const char* const TASM_DATADIR = "../data";

// Shapes with the following attribute bits are ignored for training.
// So for example if you don't want a blurred image to be used for training,
// set the AT_BadImg bit in the tag preceding its shape in the shapefile.

static const unsigned TASM_DISALLOWED_ATTR_BITS = AT_BadImg|AT_Cropped|AT_Obscured;

// When creating the mean shape aligned to the detector frame, do we
// use shapes with missing points?

static const bool TASM_MEANSHAPE_USES_SHAPES_WITH_MISSING_POINTS = false;

// About TASM_REBUILD_SUBSET:
//
// This parameter is relevant only if some training shapes have missing points
// (typically the points are not manually landmarked because they are obscured).
//
// To build a decent shape model, every point must be present in at least a
// decent sized subset of shapes.  This means that we cannot build a shape
// model purely from say the MUCT c set (because some of its landmarks
// are missing in most shapes).  So we must first build a model with the
// MUCT b and c shapes.
//
// However, once we have built the shape model with the b and c shapes
// we can impute the missing points, and then build a model with just
// the c shapes.  To do that, do the following
//
//    (i) set TASM_REBUILD_SUBSET = "c";
//
//    (ii) invoke tasm as
//            tasm muct68.shape 0 0 "i[^r].*[bc]"
//
// This builds an initial shape model with the b and c shapes and also
// imputes the missing points.  (The i[^r] part of the regex excludes the
// mirrored shapes i.e. shapes with names beginning with ir.)  It then
// rebuilds the shape model using only the c subset of those shapes.  (It
// rebuilds the shape model using the shapes with imputed points.)
//
// With the default TASM_REBUILD_SUBSET=NULL, we don't rebuild the model
// (i.e. in the above example, the model will be built from the [bc] shapes
// with imputed points).

static const char* const TASM_REBUILD_SUBSET = NULL;
// static const char* const TASM_REBUILD_SUBSET = "c";

// The following parameter applies only if TASM_REBUILD_SUBSET is not NULL.
//
// If we use a subset to rebuild the shape model, must we use the same
// subset to build the descriptor models?
//
// For example, in the scenario described above, do we build the decriptor
// models from the b and c shapes (TASM_SUBSET_DESC_MODS = true)
// or just from the c shapes (TASM_SUBSET_DESC_MODS = false);

static const bool TASM_SUBSET_DESC_MODS = true;

// When creating facedet.shape, this is the minwidth arg to DetectFaces_
// member function.  We want this as big as possible (to minimize false
// detects) but want it small enough to detect all faces in the training set.

static const int TASM_FACEDET_MINWIDTH = 25; // as a percentage

// When creating facedet.shape, should we also call the eye and mouth
// detectors and include them in facedet.shape?  Note that this does not
// affect the models built by Tasm.  It is purely for the eye and mouth
// positions in facedet.shape, which may be useful for other purposes
// outside Tasm.

static const bool TASM_DET_EYES_AND_MOUTH = true;

// Number of elements in 1D profiles.  Must be odd.
// (1D profiles will be generated for points not marked
// with AT_Hat in LANDMARK_INFO_TAB.)

static const int TASM_1D_PROFLEN = 9;

// Constants for generating the negative training cases (i.e. descriptors
// offset from the manual landmark position).  These are currently used
// only for HAT descriptors (they are not needed for classical descriptors
// because we use mahalanobis distances for classical descriptors).
//
// The x and y offset from the landmark position is chosen randomly but
// will always be >= TASM_NEG_MIN_OFFSET and <= TASM_NEG_MAX_OFFSET
//
// For linear models, TASM_NNEG_TRAIN_DESCS = 1 gives almost identical
// landmark search results as 3, and make training faster.

static const int TASM_NNEG_TRAIN_DESCS = 1; // nbr of neg descs per landmark per shape
static const int TASM_NEG_MIN_OFFSET = 1;   // min x or y displacement of a neg desc
static const int TASM_NEG_MAX_OFFSET = 6;   // max x or y displacement of a neg desc

static const int TASM_NEGTRAIN_SEED = 2013; // seed for random generation of neg offsets

// Must we duplicate the positive descriptors so there are equal numbers of
// positive and negative training descriptors, to avoid problems with
// imbalanced data?
// Balancing seems to be necessary for SVM and MARS models --- but for linear
// models, unbalanced data seems ok and training is slightly faster.  This
// constant is pertinent only if TASM_NNEG_TRAIN_DESCS > 1.

static const bool TASM_BALANCE_NUMBER_OF_CASES = false;

// When regressing,  treat all negative training descriptors offset
// distances as 1, regardless of the actual distance?
// With linear models, either value seems to give equivalent models.

static const int TASM_BINARY_DISTANCE = false;

bool IsTasmHatDesc( // true if the given point uses a HAT descriptor (not a classical descriptor)
    int ilev,       // in: pyramid level (0 is full scale)
    int ipoint);    // in

bool FaceDetFalsePos(     // true if given detpar is wrong (off the shape)
    const DetPar& detpar, // in
    const Shape&  shape); // in

bool PointUsableForTraining( // true if a point is not obscured or otherwise bad
    const Shape& shape,      // in
    int          ipoint,     // in
    unsigned     tagbits,    // in: tagbits from tag field in shape file
    const char*  base);      // in

} // namespace stasm
#endif // stasm_tasmconf_hpp
