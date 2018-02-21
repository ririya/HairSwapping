// marki.cpp: manually landmark images
//
// PRELIMINARY VERSION for Stasm 340
//
// My windows knowledge comes from Petzold's books, so that's where
// you want to look to understand this program.
//
// This file uses a form of Hungarian notation -- the prefix "g" on
// an identifier means "global" for example.
//
// Copyright (C) 2005-2013, Stephen Milborrow

#include "stasm.hpp"
#include "marki.hpp"
#include "winutil.hpp"
#include "getzoom.hpp"

// Note that the following are defined in winutil.cpp:
//    hgInstance, hgMainWnd, sgAppName

static const char   *sgMarkiLog     = "marki.log";
static const char   *sgRegistryKey  = "Software\\Marki";
static const char   *sgRegistryName = "Config";
static const int    ngDlgWidth  = 230; // dialog window width and height
static const int    ngDlgHeight = 300;
// Number of pixels allowed for toolbar at top of child windows, bigger than
// necessary to minimize misclicks onto the image (actual toolbar height is 18).
static const int    ngToolbarHeight = 32;
static const char   *sgNotifySound = "C:/Windows/Media/notify.wav";
static const int    ngMaxNbrLandmarks = 77;

// The four windows created by Marki are:
//
// 1. The main window (hgMainWnd). Mostly invisible (under the toolbar and
//    child window).
//
// 2. The toolbar (hgToolbar) with its buttons,
//
// 3. The child window (hgChildWnd).  This covers the entire main windows
//    except for the tool bar area.  We display the image in hgChildWnd.
//
// 4. The dialog window (hgDlgWnd) which typically sits to the side somewhere.

static HWND       hgToolbar;

static HWND       hgChildWnd;                  // child window, displays the image
static MFLOAT     gChildWidth, gChildHeight;   // window dims, for effecient mouse moves
static const char sgChildWnd[] = "MarkiChild"; // name and class of child window

static HWND       hgDlgWnd;     // dialog window
static int        xgDlg, ygDlg; // position of dialog window
static HBITMAP    hgToolbarBmp; // toolbar bitmap

static HCURSOR    hgGrayCursor; // the cursor we use when fgGray is true

// We store three images: the image we display, and two other
// images for speed (see LoadCurrentImg).  The images are:
//
// 1. gImg            The current image with annotations.
//
// 2. RawCurrentImg   A copy of the current image before annotations.
//                    (This is local to LoadCurrentImg.)
//
// 3. gPreloadedImg   The (unannotated) "next" image (igImg+1 if paging forward).
//                    When deciding what image to preload, we assume that the
//                    user will repeat what she has just done (and set
//                    ngPreloadOffset accordingly, will be 1 if paging forward).

static CIMAGE gImg;         // current image
static int    igImg = -1;   // index of current image in gTags
static int    ngImgs;       // total number of images
static ATTR   ugAttr;       // attribute bits of the current image (from tag)

static CIMAGE gPreloadedImg;       // preloaded image, see LoadCurrentImg
static int    igPreloadedImg = -1; // index of preloaded image, -1 means none
static int    ngPreloadOffset = 1; // estimated index offset of next image

// This flag inhibits an image copy in LoadCurrentImg (for speed
// when you are just paging through the images).
static bool  fgPrepareRawImg = true;

static char  sgShapeFile[SLEN]; // name of the shape file
static char  sgImgDirs[SLEN];   // director(ies) holding images (from shapefile)

static vec_SHAPE    gShapesOrg; // original training shapes, possibly compacted
                                // array[ngShapes] of SHAPE
                                // each SHAPE is nPoints x 2

static vec_SHAPE    gShapes;    // ditto, but modified by user mouse clicks

static const MFLOAT MARKED = INVALID;
static vec_SHAPE    gMarked;    // track which points marked by user, only IX used

static vec_string   gTags;      // array[ngShapes] of string preceding each shape
                                // in .asm file, i.e. image filenames
// command line flags

static bool fgAllButtons;               // -B
static bool fgUseRegEntries = true;     // -F
static char sgTagRegex[MAX_PRINT_LEN];  // -p
static ATTR ugMask0, ugMask1;           // -P
static ATTR ugTagAttr;                  // -t
static bool fgViewMode;                 // -V
static char sgNewShapeFile[SLEN];       // -o

static unsigned ugShowDetData;
static bool fgShowPoseData;

static MAT  gPoseSpec(1,6);             // -q
static bool fgPoseSpecifiedByUser;
static bool fgInvertPoseSpec;           // true if bang after -q

// The default situation is that the the -H and -h flags are not used,
// fgBigH is false, fgBigHLandmarks is all false,  and you can go
// to ANY landmark with the next and previous landmark buttons.
// Note that the onscreen dots for the -h and -H landmarks (if any) are always
// displayed, regardless of the IDM_ShowAllDots button (fgShowAllDots).

static bool fgBigH;                             // -H flag was used
static bool fgBigHLandmarks[ngMaxNbrLandmarks]; // landmarks selected by -H

static bool fgSmallH;                             // -h flag was used
static bool fgSmallHLandmarks[ngMaxNbrLandmarks]; // landmarks selected by -h

// check mode (initiated by command line flag -C)

static bool fgCheckMode;         // check mode on (user started Marki with -C)
static bool fgPauseCheck;        // pause -C check with space bar
static int  igFirstImg;          // first image checked
static int  ngBadImgs;           // number of bad images so far
static int  ngBadEyeMouth;       // number of images could not estimate eye-mouth dist
static int  ngBadShapes;
static char sgFirstBadImg[SLEN]; // name of first bad image, if any
static char sgFirstBadShape[SLEN];

// "Special points" flag is set if any x values in shape file
// have had SPECIAL_X_OFFSET added to them.  This is used for displaying
// results from compareworkers.exe i.e. results of multiple manual
// landmarks on the same point, with the mean point as a special point.
// We display the points in dull red, with the mean point in green.

static bool fgSpecialPoints; // true if there any special points
static MAT  gSpecialPoints;  // points which had SPECIAL_X_OFFSET offset

// toolbar buttons

static bool fgLock;
static bool fgGray;
static bool fgDark;
static bool fgShowActiveNbr = true; // show index of the active landmark
static bool fgShowAllDots = true;   // IDM_ShowAllDots
static bool fgConnectDots = true;
static bool fgShowNbrs;
static bool fgAutoNextImg;
static bool fgAutoNextPoint;        // requires fgAllButtons = true
static bool fgJumpToLandmark;
static int  igLastClosestLandmark = -1; // to minimize redisplays when jumping

static bool fgShiftKeyDown;     // set when the shift key is down
static bool fgCtrlKeyDown;      // set when the control key is down
static bool fgLButtonDown;      // true while left mouse button is down
static bool fgPendingMarkMsg;   // need to issue "Mark" msg for active landmark
static bool fgTrace;            // Ctrl-T, for debugging
static int  igDebug;            // used only when fgTrace is on
static bool fgChangesSaved = true; // landmark changes saved?

static int  igLandmark = -1;        // the active landmark
static int  igCmdLineLandmark = -1; // landmark specified by -l cmd line flag
static int  igCenterLandmark = -1;  // landmark specified by -c cmd line flag
static int  igAutoNextMissingLandmark = -1; // currently always -1
static int  ngLandmarksUpdated;     // number of landmarks updated by the user
static bool fgMoreThanOneLandmarkNbrChanged;
static bool fgSmallPoints;

static clock_t  gStartTime;     // the time this program started

// if a mouse click activates window, don't use the click to change landmark
static bool fgIgnoreClick = false;

static MFLOAT xgMouse, ygMouse; // mouse position in child window coords

// gZoomAmount is in eye-mouth distance units.
//
// For example, gZoomAmount=1 means width of image is the eye-mouth distance.
// and gZoomAmount=.5 is twice the eye-mouth distance. Note that
// we display we display 1+gZoomAmount in the main window title.

static bool     fgCenter;       // true if must center active landmark
static MFLOAT   gZoomAmount=.5; // amount of zoom, applicable only if fgCenter
static MFLOAT   gEyeMouthDist;  // eye-mouth distance of current face
static MFLOAT   gZoomedWidth;   // the width of the image rectangle to display

static int ngSmoothAmount = 1;
static const int ngEyeMouthDistForSmooth = 80; // will blur if dist less than this
static bool fgSmoothInEffect;    // set dynamically

static bool fgEqualize;

// We are careful about when we update the eye-mouth distance and zoomed
// width --- else when the user changes an eye or mouth landmark the zoomed
// width changes and the image shifts as she changes the landmark.
// This variable controls that.

static bool fgMustGetZoomedWidth; // must update gEyeMouthDist and gZoomedWidth

// Parameters for StretchDIBits.  These are global so the mouse handling
// routines can convert the mouse coords on the screen to image coords.

static MFLOAT gDestHeight, gDestWidth;
static MFLOAT xgSrc, ygSrc, gSrcWidth, gSrcHeight;
static MFLOAT xgLandmark, ygLandmark; // shape coords of currrent landmark
// fgForceRecenter tells UpdateGlobalCoords to recenter the child window
// around the current landmark.  Only needed when auto jumping to landmark.
static bool fgForceRecenter;

static bool fgGoBackOnEsc = true; // when user pushes ESC do we go back or forward?
static bool fgIssueEscMsg;

static TBBUTTON gToolbarButtons[] =
{
14, IDM_Save,          BUTTON_Standard, // change WmNotifyMarki if you move this
18, IDM_Lock,          BUTTON_Standard,
29, IDM_Blank,         BUTTON_Standard,
15, IDM_Undo,          BUTTON_Standard,
16, IDM_Redo,          BUTTON_Standard,
31, IDM_Revert,        BUTTON_Standard,
29, IDM_Blank,         BUTTON_Standard,
23, IDM_Gray,          BUTTON_Standard,
25, IDM_Dark,          BUTTON_Standard,
27, IDM_ShowActiveNbr, BUTTON_Standard,
28, IDM_ShowAllDots,   BUTTON_Standard,
 8, IDM_ConnectDots,   BUTTON_Standard,
10, IDM_ShowNbrs,      BUTTON_Standard,
 6, IDM_WriteImg,      BUTTON_Standard,
29, IDM_Blank,         BUTTON_Standard,
 5, IDM_Center,        BUTTON_Standard,
 3, IDM_ZoomOut,       BUTTON_Standard,
 4, IDM_ZoomIn,        BUTTON_Standard,
29, IDM_Blank,         BUTTON_Standard,
 2, IDM_AutoNextImg,   BUTTON_Standard,
 0, IDM_PrevImg,       BUTTON_Standard,
 1, IDM_NextImg,       BUTTON_Standard,
29, IDM_Blank,         BUTTON_Standard,
17, IDM_Help,          BUTTON_Standard,
-1, // -1 terminates list
};

static TBBUTTON gToolbarButtonsAll[] = // use if -B flag is in effect
{
14, IDM_Save,           BUTTON_Standard, // change WmNotifyMarki if you move this
18, IDM_Lock,           BUTTON_Standard,
29, IDM_Blank,          BUTTON_Standard,
15, IDM_Undo,           BUTTON_Standard,
16, IDM_Redo,           BUTTON_Standard,
31, IDM_Revert,         BUTTON_Standard,
29, IDM_Blank,          BUTTON_Standard,
23, IDM_Gray,           BUTTON_Standard,
25, IDM_Dark,           BUTTON_Standard,
27, IDM_ShowActiveNbr,  BUTTON_Standard,
28, IDM_ShowAllDots,    BUTTON_Standard,
 8, IDM_ConnectDots,    BUTTON_Standard,
10, IDM_ShowNbrs,       BUTTON_Standard,
 6, IDM_WriteImg,       BUTTON_Standard,
29, IDM_Blank,          BUTTON_Standard,
 5, IDM_Center,         BUTTON_Standard,
 3, IDM_ZoomOut,        BUTTON_Standard,
 4, IDM_ZoomIn,         BUTTON_Standard,
29, IDM_Blank,          BUTTON_Standard,
 2, IDM_AutoNextImg,    BUTTON_Standard,
 0, IDM_PrevImg,        BUTTON_Standard,
 1, IDM_NextImg,        BUTTON_Standard,
29, IDM_Blank,          BUTTON_Standard,
// 11, IDM_AutoNextPoint,  BUTTON_Standard, // TODO always disable auto-next-point
12, IDM_PrevPoint,      BUTTON_Standard,
13, IDM_NextPoint,      BUTTON_Standard,
30, IDM_JumpToLandmark, BUTTON_Standard,
29, IDM_Blank,          BUTTON_Standard,
17, IDM_Help,           BUTTON_Standard,
-1, // -1 terminates list
};

// this must be ordered as the numbers of IDM_Save etc. in marki.hpp
static const char *sgTooltips[] =
{
" Save landmarks to shape file ",                   // IDM_Save
" Lock (disables left mouse click) ",               // IDM_Lock
" Undo ",                                           // IDM_Undo
" Redo ",                                           // IDM_Redo
" Undo all changes to this image ",                 // IDM_Revert
" Color or gray image? \n\n Keyboard A ",           // IDM_Gray
" Darken image? \n\n Keyboard S ",                  // IDM_Dark
" Show active landmark number? \n\n Keyboard D ",   // IDM_ShowActiveNbr
" Show all landmarks? \n\n Keyboard F ",            // IDM_ShowAllDots
" Connect the dots? Keyboard G ",                   // IDM_ConnectDots
" Show landmark numbers? \n\nKeyboard H ",          // IDM_ShowNbrs
" Write image to disk ",                            // IDM_WriteImg
" Center active landmark? \n\n Keyboard Z ",        // IDM_Center
" Zoom out \n (and turn on centering if not on) \n\n Keyboard X", // IDM_ZoomOut
" Zoom in \n (and turn on centering if not on) \n\n Keyboard C",  // IDM_ZoomIn
" Auto next image after left mouse click? ",        // IDM_AutoNextImg
" Previous image \n\n Keyboard PageUp ",            // IDM_PrevImg
" Next image \n\n Keyboard SPACE or PageDown ",     // IDM_NextImg
" Auto next landmark after left mouse click ",      // IDM_AutoNextPoint
" Previous landmark ",                              // IDM_PrevPoint
" Next landmark ",                                  // IDM_NextPoint
" Auto jump to landmark\nRight click twice to disable ",
                                                    // IDM_JumpToLandmark
" Help",                                            // IDM_Help
"",                                                 // IDM_Blank
};

static const char sgUsage[] =
"Usage:  marki  [FLAGS]  file.shape\n"
"\n"
"Example 1:  marki  myshapes.shape    (will save results to the same file)\n"
"\n"
"Example 2:  marki  -o temp.shape  -p \" i[^r]\"  ../data/muct68.shape\n"
"\n"
"-B\t\tShow all buttons\n"
"\t\tAllows you to change the active landmark number by clicking on buttons\n"
"\n"
"-C\t\tCheck that all images in the shape file are accessible\n"
"\n"
"-F\t\tFresh start (ignore saved window positions etc.)\n"
"\n"
"-h 31 42 ...\tHighlight landmarks with indices 31 42 ... (* for all)\n"
"\n"
"-H 31 42 ...\tShow all buttons, allow selection only of 31 42 ...\n"
"\n"
"-c LANDMARK\tCenter landmark (else center on active landmark)\n"
"\n"
"-l LANDMARK\tLandmark to be altered by left mouse click\n"
"\t\tDefault: landmark used last time Marki was used\n"
"\n"
#if 0
"-m LANDMARK\t\"AutoNext\" to next image with this landmark missing\n"
"\n"
#endif
"-o OUT.shape\tOutput filename\n"
"\t\tDefault: overwrite the original file\n"
"\n"
"-p REGEX\t\tLoad shapes which match case-independent REGEX\n"
"\t\tExample: -p \"xyz\" loads filenames containing xyz\n"
"\t\tExample: -p \" i000| i001\" loads filenames beginning with i000 or i001\n"
"\t\tDefault: all (except face detector shapes)\n"
"\n"
"-P Mask0 Mask1\tLoad shapes which satisfy (Attr & Mask0) == Mask1\n"
"\t\tMask1=ffffffff is treated specially, meaning satisfy (Attr & Mask0) != 0\n"
"\t\tAttr is the hex number at start of the tag, Mask0 and Mask1 are hex\n"
"\t\tThis filter is applied after -p REGEX\n"
"\t\tExample: -P 2 2 matches faces with glasses (AT_Glasses=2, see atface.hpp)\n"
"\t\tExample: -P 2 0 matches faces without glasses\n"
"\t\tDefault: no filter (Mask0=Mask1=0)\n"
"\n"
"-q[!] YawMin YawMax PitchMin PitchMax RollMin RollMax\n"
"\t\tSelect shapes in the given pose range, 0 0 means any\n"
"\t\tIf bang after -q then invert selection\n"
"\n"
"-t Mask\t\tTag mode\n"
"\t\tLeft-click or left-arrow clears Mask bit\n"
"\t\tRight-click or right-arrow sets it\n"
"\n"
"-V\t\tView mode (no landmarks changes allowed)";

static const char sgHelp[] =
"LeftMouse    change position of landmark\n"
"RightMouse  toggle darken face (and disable \"auto jump to landmark\")\n"
"ScrollMouse  next/previous image\n"
"\n"
"PgDown or SPACE  next image        PgUp  previous image\n"
"\n"
"Arrow Keys   next/previous image\n"
"\n"
"Home  first image                             End  last image\n"
"\n"
"Hold Shift at same time to repeat above commands 10 times\n"
"e.g. Shift-SPACE jumps 10 images forward\n"
"\n"
"Also: Ctrl-PgDown pages down to 10% boundary\n"
"          Ctrl-PgUp up to 10% boundary\n"
"          Ctrl-Space previous image\n"
"\n"
"TAB  mark point as unused\n"
"Q      equalize image\n"
"W     change smooth amount\n"
"\n"
"A  color image?\n"
"S  darken the face?\n"
"D  show active landmark number?\n"
"F  hide original landmarks?\n"
"G  connect the dots?\n"
"H  show landmark numbers?\n"
".    small points and lines\n"
"\n"
"Z  center active landmark?\n"
"X  zoom out (when centered)\n"
"C  zoom in (when centered)\n"
"V  zoom out two levels (when centered)\n"
"B  zoom in two levels (when centered)\n"
"\n"
"Control keys:\n"
"          ESC     quick look at previous image\n"
"          Ctrl-C  quit\n"
"          Ctrl-N  next landmark            Ctrl-P   previous landmark\n"
"          Ctrl-Z   toggle undo/redo        Ctrl-T   trace\n"
"          Metadata: Ctrl-Q all     Ctrl-W Yaw00\n"
"                    Ctrl-E Yaw22 and Yaw-22\n"
"                    Ctrl-R Yaw45 and Yaw-45\n";

static void ChildWm_Paint(HWND hWnd);
static void MouseToImgCoords(MFLOAT &x, MFLOAT &y,
                             MFLOAT xMouse, MFLOAT yMouse);
static void DisplayButtons(void);
static void DisplayTitle(bool fWmMouseMove);
static void IssueCheckModeResults(void);
static void SaveStateToRegistry(void);
static void GetStateFromRegistry(int *pxPos, int *pyPos,       // out
                                 int *pxSize, int *pySize,     // out
                                 int *pxDlg, int *pyDlg);      // out
static void Redisplay(void);
static void SetImgNbr(int iImg);
static void SetLandmarkNbr(int iLandmark);
static void SetLandmarkPosition(int iImg, int iLandmark, MFLOAT x, MFLOAT y,
                                bool fsueMsgNow);
static void TagCurrentShape(bool fTag);
static void NotifyUserIfWrappedToFirstImg(void);

//-----------------------------------------------------------------------------
// like printf, but prints to the log file with a time stamp as well,
// if the log file is open

void tprintf (const char *pArgs, ...)   // args like printf
{
static char s[MAX_PRINT_LEN];
va_list pArg;
va_start(pArg, pArgs);
vsprintf(s, pArgs, pArg);
va_end(pArg);
printf("%s", s);
fflush(stdout);     // flush so if there is a crash we can see what happened
if(pgLogFile)
    {
    if(fprintf(pgLogFile, "%6.6d: %s",
            (clock() - gStartTime) / CLOCKS_PER_SEC, s) <= 0)
        Err1("Cannot write to the log file (string %s)", s);
    fflush(pgLogFile);
    }
}

//-----------------------------------------------------------------------------
static const char *sGetCurrentBase (void) // basename of current image
{
if(NSIZE(gTags) == 0)
    return "?";
return sGetBaseFromTag(gTags[igImg]);
}

//-----------------------------------------------------------------------------
static const char *sGetFirstBase (void) // basename of first image loaded, for -C
{
if(NSIZE(gTags) == 0)
    return "?";
return sGetBaseFromTag(gTags[igFirstImg]);
}

//-----------------------------------------------------------------------------
static bool fFaceDetMat (void)
{
return (ugAttr & AT_DetYaw00) != 0;
}

//-----------------------------------------------------------------------------
// This is called if gShapes[igImg] is not actually a shape --- it is a
// matrix of face detector results.  Convert these to a shape so we can
// display them.

static SHAPE ProcessFaceDetMat (void)
{
fgCenter = false;   // centering illegal for face det mats
DET_PARAMS DetParams(ShapeFileEntryAsDetParams(gShapes[igImg]));
if(!fValid(DetParams.Rot))
    DetParams.Rot = 0;
SHAPE DetShape(DetParamsAsShape(DetParams));
CropShapeToImg(DetShape, gImg);
return DetShape;
}

//-----------------------------------------------------------------------------
static void EqualizeImgUnderShape (
    IplImage    &IplImg,  // io
    const SHAPE &Shape,
    MFLOAT      Extend=0) // extend shape
{
// TODO this gives lousy results
CV_Assert(IplImg.nChannels == 3);
IplImage *hsv = cvCreateImage(cvGetSize(&IplImg), 8, 3); CV_Assert(hsv);
IplImage *h   = cvCreateImage(cvGetSize(&IplImg), 8, 1); CV_Assert(h);
IplImage *s   = cvCreateImage(cvGetSize(&IplImg), 8, 1); CV_Assert(s);
IplImage *v   = cvCreateImage(cvGetSize(&IplImg), 8, 1); CV_Assert(v);
cvCvtColor(&IplImg, hsv, CV_BGR2HSV);
cvSplit(hsv, h, s, v, NULL);
MFLOAT xMin = ShapeMinX(Shape);
MFLOAT xMax = ShapeMaxX(Shape);
MFLOAT yMin = ShapeMinY(Shape);
MFLOAT yMax = ShapeMaxY(Shape);
MFLOAT width = xMax - xMin; MFLOAT xAdj = Extend * width;
MFLOAT height = yMin - yMax; MFLOAT yAdj = Extend * height;
cvSetImageROI(v,
    cvRect(cvRound(xMin-xAdj),    cvRound(yMax-yAdj),
           cvRound(width+2*xAdj), cvRound(height+2*yAdj)));
cvEqualizeHist(v, v);
cvResetImageROI(v);
cvMerge(h, s, v, NULL, hsv);
cvCvtColor(hsv, &IplImg, CV_HSV2BGR);
CV_Assert(IplImg.nChannels == 3);
cvReleaseImage(&hsv);
cvReleaseImage(&h);
cvReleaseImage(&s);
cvReleaseImage(&v);
}

//-----------------------------------------------------------------------------
// Possibly apply smoothing and equalization to the image

static void PreprocessImg (void)
{
fgSmoothInEffect = ngSmoothAmount && gEyeMouthDist <= ngEyeMouthDistForSmooth;
if(fgSmoothInEffect || fgEqualize)
    {
    // Following type conversion is needed because there is no Mat based
    // cvSmooth in OpenCV 2.3.1.
    // Just the pointer is copied, not the buffer contents.
    IplImage IplImg = gImg;
    if(fgSmoothInEffect)
        {
        int nKernel = 2 * ngSmoothAmount - 1; // must be an odd number
        cvSmooth(&IplImg, &IplImg, CV_GAUSSIAN, nKernel, nKernel);
        }
    if(fgEqualize)
        EqualizeImgUnderShape(IplImg, gShapes[igImg], .25);
    }
}

//-----------------------------------------------------------------------------
static void DrawPointOrX (int    iLandmark,
                          byte   Red,
                          byte   Green,
                          byte   Blue,
                          double Size,
                          bool   fUseShapesOrg=false)
{
SHAPE Shape(fUseShapesOrg? gShapesOrg[igImg]: gShapes[igImg]);

MFLOAT x, y; GetPointCoordsFromShape(x, y, Shape, iLandmark, true);

if(fPointUsed(Shape, iLandmark))
    DrawPoint(gImg, x, y, Red, Green, Blue, 1.3 * Size);
else
    DrawX(gImg, x, y, Red, Green, Blue, 1.3 * Size);
}

//-----------------------------------------------------------------------------
static void AnnotateSpecialPoints (double Size,     // in
                                   double FontSize) // in
{
for(int iPoint = 0; iPoint < gShapes[igImg].rows; iPoint++)
    {
    MFLOAT x, y; GetPointCoordsFromShape(x, y, gShapes[igImg], iPoint, true);
    if(gSpecialPoints(iPoint))   // draw special points in green
        DrawPoint(gImg, x, y, 0x00, 0xff, 0x00, Size);
    else                    // other points in dark red
        DrawPoint(gImg, x, y, 0x7f, 0x00, 0x00, Size);
    }

if(fgShowActiveNbr) // show the active landmark number in yellow?
    DrawShape(gImg, gShapes[igImg], C_NONE, false, C_NONE, false,
              igLandmark, true, Size, FontSize);
}

//-----------------------------------------------------------------------------
static void DrawFaceDetShape ( // draws onto gImg
    const DET_PARAMS &DetParams, // in
    const COL        Col,        // in
    const char       sYaw[],     // in
    MFLOAT           Yaw,        // in: estimated av yaw
    double           FontSize)   // in
{
const SHAPE DetShape(DetParamsAsShape(DetParams));
DrawShape(gImg, DetShape, C_NONE, true, Col, false, -1, true);
ImgPrintf(gImg, DetShape(0, IX), DetShape(0, IY) - MIN(5, gZoomedWidth / 100),
          Col, FontSize, "%s%s",
          sYaw, fValid(Yaw)? ssprintf("  Yaw%g", Yaw): "");
if(fPointUsed(DetShape, 5)) // left eye available?
    ImgPrintf(gImg, DetShape(5, IX)+5, DetShape(5, IY), Col, -1, "L");
if(fPointUsed(DetShape, 6)) // right eye available?
    ImgPrintf(gImg, DetShape(6, IX)+5, DetShape(6, IY), Col, -1, "R");
}

//-----------------------------------------------------------------------------
// This modifies gImg by possibly converting to it gray, adding the landmark
// annotations, adding the active landmark number etc.

static void AnnotateImg (void)
{
PreprocessImg();
if(fgGray)
    DesaturateImg(gImg);
if(fgDark)
    DarkenImg(gImg);

double Size = double(cvRound(gZoomedWidth / 300)); // size of displayed points
if(gZoomedWidth > 2000 && Size < 7)
    Size = 7;                               // force big for big images
if(gZoomedWidth > 200 && Size < 2 && gEyeMouthDist > 220)
    Size = 2;
if(fgSmallPoints)
    Size = 1;

double FontSize = gZoomedWidth / 1200;
if(FontSize > 2)
    FontSize = 2;
if(fgSmallPoints)
    FontSize /= 2;

ATTR uAttr = uGetAttrFromTag(gTags[igImg]);

if(fgSpecialPoints)
    {
    AnnotateSpecialPoints(Size, FontSize);
    return;                                 // NOTE: return
    }
SHAPE Shape;
if(uAttr & AT_DetYaw00)
    Shape = ProcessFaceDetMat();
else
    Shape = gShapes[igImg];

if(fgShowActiveNbr || fgShowAllDots || fgConnectDots || fgShowNbrs)
    {
    if(uAttr & AT_DetYaw00)
        DrawShape(gImg,
                  Shape, C_YELLOW, fgConnectDots, Dark(C_YELLOW), fgShowNbrs,
                  -1, true, 1.5 * Size, FontSize);
    else // normal shape
        {
        DrawShape(gImg, Shape,
            (fgShowAllDots || fgShowNbrs)? C_RED: C_NONE,
            fgConnectDots, Dark(C_RED), fgShowNbrs,
            fgShowActiveNbr? igLandmark: -1, true,
            Size, FontSize);

        if(fgShowAllDots)
            {
            // draw old mark in red or orange
            if(gMarked[igImg](igLandmark, IX) == MARKED) // orange
                DrawPointOrX(igLandmark, 0xc0, 0x80, 0x00, Size, true);
            else                                         // bright red
                DrawPointOrX(igLandmark, 0xff, 0x20, 0x20, Size, true);
            }
        }
    }

int i;
for(i = 0; i < gShapes[igImg].rows; i++)
    {
    if(gMarked[igImg](i, IX) == MARKED)
        {
        // draw active point if marked in darkish cyan or bright cyan
        if(i == igLandmark)  // bright cyan
            DrawPointOrX(i, 0x20, 0xff, 0xff, Size);
        else                 // darkish cyan
            DrawPointOrX(i, 0x00, 0xd0, 0xd0, Size);
        }
    else if(i < ngMaxNbrLandmarks && (fgSmallHLandmarks[i] || fgBigHLandmarks[i]))
    {
        lprintf("-h or -H landmark: i %d %d %d\n", i, fgSmallHLandmarks[i], fgBigHLandmarks[i]);
        DrawPointOrX(i, 255, 255, 0, Size);           // yellow
    }
    else if(fgShowAllDots && // handle point specified by -c flag
            (i != igLandmark && i == igCenterLandmark))
        DrawPointOrX(i, 255, 255, 255, Size);         // white
    }

// show unused points in green
for(i = 0; i < Shape.rows; i++)
    if(!fPointUsed(Shape, i))
        DrawPointOrX(i, 0x00, 0xff, 0x00, Size, false);

DET_PARAMS DetParams;
if((ugShowDetData == AT_DetYaw00 || ugShowDetData == AT_Meta) && // AT_Meta means all det data
    fGetDetParamsFromShapeFile(DetParams, sGetCurrentBase(), sgShapeFile, AT_DetYaw00))
    {
    DrawFaceDetShape(DetParams, YawEnumAsCol(YAW00), sYawEnumAsString(YAW00), DetParams.Yaw, FontSize);
    }
if(ugShowDetData == (AT_DetYaw22|AT_DetYaw_22) || ugShowDetData == AT_Meta)
    {
    if(fGetDetParamsFromShapeFile(DetParams, sGetCurrentBase(), sgShapeFile, AT_DetYaw22))
        DrawFaceDetShape(DetParams, YawEnumAsCol(YAW22), sYawEnumAsString(YAW22), DetParams.Yaw, FontSize);
    if(fGetDetParamsFromShapeFile(DetParams, sGetCurrentBase(), sgShapeFile, AT_DetYaw_22))
        DrawFaceDetShape(DetParams, YawEnumAsCol(YAW_22), sYawEnumAsString(YAW_22), DetParams.Yaw, FontSize);
    }
if(ugShowDetData == (AT_DetYaw45|AT_DetYaw_45) || ugShowDetData == AT_Meta)
    {
    if(fGetDetParamsFromShapeFile(DetParams, sGetCurrentBase(), sgShapeFile, AT_DetYaw45))
        DrawFaceDetShape(DetParams, YawEnumAsCol(YAW45), sYawEnumAsString(YAW45), DetParams.Yaw, FontSize);
    if(fGetDetParamsFromShapeFile(DetParams, sGetCurrentBase(), sgShapeFile, AT_DetYaw_45))
        DrawFaceDetShape(DetParams, YawEnumAsCol(YAW_45), sYawEnumAsString(YAW_45), DetParams.Yaw, FontSize);
    }
}

//-----------------------------------------------------------------------------
// preload the next image

static void PreloadImg (void)
{
int iImg = igImg + ngPreloadOffset; // the image we will preload

if(iImg >= ngImgs) iImg = 0; else if(iImg < 0) iImg = ngImgs-1; // wrap

const char *sBase = sGetBaseFromTag(gTags[iImg]);

if(iImg != igPreloadedImg &&        // not already loaded?
    NULL == sLoadImgGivenDirs(gPreloadedImg,
                              sBase, sgImgDirs, sgShapeFile, NO_EXIT_ON_ERR))
    {
     // successfully loaded
    igPreloadedImg = iImg;
    if(fgTrace)
        tprintf("                                   "
                "Preload %d %s\n", igPreloadedImg, sBase);
    }
}

//-----------------------------------------------------------------------------
// This func is called by Redisplay and during initialization in WinMain.
// On entry, igImg is set to the image we want to load.
//
// There are two levels of caching, as follows.
//
// gPreloadedImg: This caching is optimized for single clicks with
// fgAutoNextImg enabled i.e. it is optimized for igImg increasing by 1.
// We prepare it as follows. Immediately after loading and displaying the
// current image, we preload the next image.  So when the user clicks and
// releases the mouse, the image is ready.
//
// RawImg: This caching is optimized for when the current image is
// modified (e.g. if user changes fGray or the igLandmark but with the same
// igImg, i.e. the image basename does not change).  RawCurrentImg is the raw
// current image, before annotations (i.e. before landmark points etc. are
// painted in).

static void LoadCurrentImg (void)
{
static CIMAGE RawImg;                  // current image, before annotations
static int iRawImg = -1;
static bool fLoadedAnImg;              // true after loading at least one image
const char *sErr = NULL;
const char *sBase = sGetCurrentBase(); // base name of image at igImg
bool fSameImg = false;                 // only want one eye warn per image
igDebug++;                             // used only when fgTrace is on

if(igImg == iRawImg) // igImg didn't change?
    {
    // igImg did not change i.e. use the same image but possibly want
    // different annotation on the image. We copy the entire
    // gImg buffer so future changes to gImg don't change RawImg.
    RawImg.copyTo(gImg);
    fSameImg = true;
    if(fgTrace)
        lprintf("%d Use RawImg %d %s\n", igDebug, iRawImg, sBase);
    }
else if(igImg == igPreloadedImg) // igImg changed but preload is available
    {
    gPreloadedImg.copyTo(gImg);
    if(fgPrepareRawImg)
        {
        gImg.copyTo(RawImg);
        iRawImg = igImg;
        }
    else
        iRawImg = -1;   // mark raw image as not useable since not copied above
    fgMustGetZoomedWidth = true; // image changed so must get new zoom width
    if(fgTrace)
        lprintf(
            "%d Use preloaded image %d %s "
            "(with %sraw image copy for next time)\n",
            igDebug, igPreloadedImg, sBase, (iRawImg == -1)? "no ": "");
    }
else // new image (igImg changed and preload is not available)
    {
    if(fgTrace)
        lprintf("%d Load new image %d %s\n", igDebug, igImg, sBase);
    sErr = sLoadImgGivenDirs(gImg,
                             sBase, sgImgDirs, sgShapeFile, NO_EXIT_ON_ERR);
    if(sErr == NULL) // successfully loaded?
        {
        gImg.copyTo(RawImg);
        iRawImg = igImg;
        fLoadedAnImg = true;
        fgMustGetZoomedWidth = true; // image changed so new zoom width
        }
    else if(fgCheckMode)
        {
        tprintf("%s: Cannot load\n", sBase);
        if(ngBadImgs == 0)
            strcpy(sgFirstBadImg, sBase);
        ngBadImgs++;
        }
    else if(fLoadedAnImg) // previously loaded at least one good image?
        {
        TimedMsgBox("    Could not load [%d] %s    \n", igImg, sBase);
        const char *sOldBase = sBase;
        const int iOldImg = igImg;
        int iCount = 0;
        while(iCount++ < ngImgs && sErr)
            {
            igImg += ngPreloadOffset > 0? 1: -1;
            if(igImg == ngImgs)
                igImg = 0;
            else if(igImg < 0)
                igImg = ngImgs - 1;
            sBase = sGetBaseFromTag(gTags[igImg]);
            sErr = sLoadImgGivenDirs(gImg,
                sBase, sgImgDirs, sgShapeFile, NO_EXIT_ON_ERR);
            }
        if(!sErr)   // success?
            {
            TimedMsgBox("    Could not load [%d] %s    \n"
                        "Loaded [%d] %s instead",
                        iOldImg, sOldBase, igImg, sBase);
            gImg.copyTo(RawImg);
            iRawImg = igImg;
            fLoadedAnImg = true;
            fgMustGetZoomedWidth = true;
            NotifyUserIfWrappedToFirstImg();
            }
        }
    else // very first image and couldn't load it
        {
        igImg = 0; // for next time we run Marki TODO correct?
        Err1(sErr);
        PostQuitMessage(0); // not quite right but does the job
        }
    }
ugAttr = uGetAttrFromTag(gTags[igImg]); // update ugAttr attribute bits from tag
if(!sErr && fgMustGetZoomedWidth)
    {
    bool fRefPointMissing = false;
    // for special points no GetEyeMouthDist because no ToShape17 function
    if(fgSpecialPoints || gShapes[igImg].rows == 1 || gShapes[igImg].rows == 5)
        gEyeMouthDist = gImg.cols / 10;
    else
        gEyeMouthDist = cvRound(GetEyeMouthDist(gShapes[igImg], false, true));
    gZoomedWidth = GetZoomedWidth(fgCenter? gZoomAmount: 0,
                                  gEyeMouthDist, MFLOAT(gImg.cols));
    if(fgCheckMode && !fSameImg) // keep a record of images stats
        {
        MFLOAT InterEyeDist = 0, FaceDetWidth = 0;
        if(!fgSpecialPoints)
            {
            InterEyeDist = cvRound(GetInterEyeDist(gShapes[igImg], false));
            DET_PARAMS DetParams;
            // TODO is AT_DetYaw00 always correct here?
            if(fGetDetParamsFromShapeFile(DetParams, sBase, sgShapeFile, AT_DetYaw00))
                FaceDetWidth = DetParams.width;
            }
        if(gEyeMouthDist == 0)
            ngBadEyeMouth++;
        logprintf(
"%d %s: eyemouth %5.0f intereye %5.0f facedet %5.0f width %5d height %5d size %5.2f MPixel%s\n",
            igImg, sBase, gEyeMouthDist, InterEyeDist, FaceDetWidth,
            gImg.cols, gImg.rows, gImg.cols * gImg.rows / (1024. * 1024.),
            fRefPointMissing? " pupil missing": "");
        double xmin, xmax, ymin, ymax; GetShapeMinMax(xmin, xmax, ymin, ymax, gShapes[igImg]);
        if (xmin > gImg.cols || xmax < 0 || ymin > gImg.rows || ymax < 0)
            {
            tprintf("Bad shape xmin %g xmax %g ymin %g ymax %g width %d height %d\n",
                    xmin, xmax, ymin, ymax, gImg.cols, gImg.rows);
            if(ngBadShapes == 0)
                strcpy(sgFirstBadShape, sBase);
            ngBadShapes++;
            }
        }
    fgMustGetZoomedWidth = false;
    }
if(!sErr)
    AnnotateImg();
}

//-----------------------------------------------------------------------------
static void CreateMyCursor (HINSTANCE hInstance)
{
const BYTE CursorAnd[] =
{
    0xff, 0xff, 0xff, 0xff,   // 00
    0xff, 0xff, 0xff, 0xff,   // 01
    0xff, 0xfe, 0x7f, 0xff,   // 02
    0xff, 0xfe, 0x7f, 0xff,   // 03
    0xff, 0xfe, 0x7f, 0xff,   // 04
    0xff, 0xfe, 0x7f, 0xff,   // 05
    0xff, 0xfe, 0x7f, 0xff,   // 06
    0xff, 0xfe, 0x7f, 0xff,   // 07
    0xff, 0xfe, 0x7f, 0xff,   // 08
    0xff, 0xfe, 0x7f, 0xff,   // 09
    0xff, 0xfe, 0x7f, 0xff,   // 10
    0xff, 0xfe, 0x7f, 0xff,   // 11
    0xff, 0xfe, 0x7f, 0xff,   // 12
    0xff, 0xfe, 0x7f, 0xff,   // 13
    0xff, 0xff, 0xff, 0xff,   // 14

    0xc0, 0x03, 0xC0, 0x03,   // 15
    0xc0, 0x03, 0xC0, 0x03,   // 16

    0xff, 0xff, 0xff, 0xff,   // 17
    0xff, 0xfe, 0x7f, 0xff,   // 18
    0xff, 0xfe, 0x7f, 0xff,   // 19
    0xff, 0xfe, 0x7f, 0xff,   // 20
    0xff, 0xfe, 0x7f, 0xff,   // 21
    0xff, 0xfe, 0x7f, 0xff,   // 22
    0xff, 0xfe, 0x7f, 0xff,   // 23
    0xff, 0xfe, 0x7f, 0xff,   // 24
    0xff, 0xfe, 0x7f, 0xff,   // 25
    0xff, 0xfe, 0x7f, 0xff,   // 26
    0xff, 0xfe, 0x7f, 0xff,   // 27
    0xff, 0xfe, 0x7f, 0xff,   // 28
    0xff, 0xfe, 0x7f, 0xff,   // 29
    0xff, 0xff, 0xff, 0xff,   // 30
    0xff, 0xff, 0xff, 0xff    // 31
};

const BYTE CursorXor[] =
{
    0x00, 0x00, 0x00, 0x00,   // 00
    0x00, 0x00, 0x00, 0x00,   // 01
    0x00, 0x01, 0x80, 0x00,   // 02
    0x00, 0x01, 0x80, 0x00,   // 03
    0x00, 0x01, 0x80, 0x00,   // 04
    0x00, 0x01, 0x80, 0x00,   // 05
    0x00, 0x01, 0x80, 0x00,   // 06
    0x00, 0x01, 0x80, 0x00,   // 07
    0x00, 0x01, 0x80, 0x00,   // 08
    0x00, 0x01, 0x80, 0x00,   // 09
    0x00, 0x01, 0x80, 0x00,   // 10
    0x00, 0x01, 0x80, 0x00,   // 11
    0x00, 0x01, 0x80, 0x00,   // 12
    0x00, 0x01, 0x80, 0x00,   // 13
    0x00, 0x00, 0x00, 0x00,   // 14

    0x3F, 0xFC, 0x3F, 0xFC,   // 15
    0x3F, 0xFC, 0x3F, 0xFC,   // 16

    0x00, 0x00, 0x00, 0x00,   // 17
    0x00, 0x01, 0x80, 0x00,   // 18
    0x00, 0x01, 0x80, 0x00,   // 19
    0x00, 0x01, 0x80, 0x00,   // 20
    0x00, 0x01, 0x80, 0x00,   // 21
    0x00, 0x01, 0x80, 0x00,   // 22
    0x00, 0x01, 0x80, 0x00,   // 23
    0x00, 0x01, 0x80, 0x00,   // 24
    0x00, 0x01, 0x80, 0x00,   // 25
    0x00, 0x01, 0x80, 0x00,   // 26
    0x00, 0x01, 0x80, 0x00,   // 27
    0x00, 0x01, 0x80, 0x00,   // 28
    0x00, 0x01, 0x80, 0x00,   // 29
    0x00, 0x00, 0x00, 0x00,   // 30
    0x00, 0x00, 0x00, 0x00    // 31
};
hgGrayCursor = CreateCursor(
        hInstance,              // app. instance
        16,                     // horizontal position of hot spot
        16,                     // vertical position of hot spot
        32,                     // cursor width
        32,                     // cursor height
        CursorAnd,              // AND mask
        CursorXor);             // XOR mask
}

//-----------------------------------------------------------------------------
static void SetChildCursor (void)
{
// the double typecast is necessary to not get a warning in 64 bit builds
SetClassLongPtr(hgChildWnd, GCLP_HCURSOR,
                LONG(LONG_PTR(fgGray? hgGrayCursor: LoadCursor(NULL, IDC_CROSS))));
}

//-----------------------------------------------------------------------------
static void Wm_Size (HWND hWnd, LPARAM lParam)
{
SetChildCursor(); // TODO else hourglass cursor stays for a while, why?

SendMessage(hgToolbar, TB_AUTOSIZE, 0, 0L);

gChildWidth = LOWORD(lParam); // width of parent

// height of parent minus toolbar
gChildHeight = MFLOAT(MAX(1, HIWORD(lParam)- ngToolbarHeight));

MoveWindow(hgChildWnd, 0, ngToolbarHeight,
          (int)gChildWidth, (int)gChildHeight, true);
}

//-----------------------------------------------------------------------------
static void Wm_Move (HWND hWnd, LPARAM lParam)
{
// move the dialog window along with the main window
#if 0 // works, but needs work to prevent oddness when maxing and minning a wind
static int xOldPos = -9999, yOldPos;
int xPos = int(LOWORD(lParam)), yPos = int(HIWORD(lParam));
if(xOldPos != -9999) // not first time
    {
    RECT RectWorkArea;
    if(SystemParametersInfo(SPI_GETWORKAREA, 0, &RectWorkArea, 0))
        {
        RECT rectDlg; GetWindowRect(hgDlgWnd, &rectDlg);
        xgDlg = int(rectDlg.left);
        ygDlg = int(rectDlg.top);
        xgDlg += short(xPos - xOldPos);
        ygDlg += short(yPos - yOldPos);
        xgDlg = CLAMP(xgDlg, 0, RectWorkArea.right  - ngDlgWidth);
        ygDlg = CLAMP(ygDlg, 0, RectWorkArea.bottom - ngDlgHeight);
        MoveWindow(hgDlgWnd, xgDlg, ygDlg, ngDlgWidth, ngDlgHeight, true);
        }
    }
xOldPos = xPos;
yOldPos = yPos;
#endif
}

//-----------------------------------------------------------------------------
// Display a tooltip (user positioned the mouse over a toolbar button).
// Customized for marki (that's why we don't use winutil.cpp:WmNotify).

static void Wm_NotifyMarki (HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam,
                            const char **psTooltips, int iFirst)
{
if(((LPNMHDR)lParam)->code == TTN_NEEDTEXT)
    {
    CV_Assert(gToolbarButtons[0].idCommand == IDM_Save);
    // sgTooltips[0] is IDM_Save
    sgTooltips[0] = ssprintf(" Save landmarks \n\n Shape file:\n %s ", sgNewShapeFile);
    LPTOOLTIPTEXT pToolTipText = (LPTOOLTIPTEXT)lParam;
    INT_PTR       iButton = pToolTipText->hdr.idFrom - iFirst;
    LPSTR         pDest = pToolTipText->lpszText;
    strcpy(pDest, psTooltips[iButton]);
    }
}

//-----------------------------------------------------------------------------
// Functions for undo

typedef struct UNDO
    {
    int iImg;
    int iLandmark;
    MFLOAT x;
    MFLOAT y;
    }
UNDO;

static vector<UNDO> gUndo;  // undo buffer
static vector<UNDO> gRedo;  // redo buffer
static bool fgLastWasUndo;  // last command was an undo (used by Ctrl-Z handling)
static bool fgLastWasRedo;  // last command was a redo

static void SaveForUndo1 (vector<UNDO> &u)
{
bool fChanged = true;
if(NSIZE(u))
    {
    const UNDO OldUndo = u[NSIZE(u)-1];
    fChanged = OldUndo.iImg != igImg ||
               OldUndo.iLandmark != igLandmark ||
               OldUndo.x != gShapes[igImg](igLandmark, IX) ||
               OldUndo.y != gShapes[igImg](igLandmark, IY);
    }
if(fChanged)
    {
    UNDO Undo;
    Undo.iImg = igImg;
    Undo.iLandmark = igLandmark;
    Undo.x = gShapes[igImg](igLandmark, IX);
    Undo.y = gShapes[igImg](igLandmark, IY);
    u.push_back(Undo);
    }
}

static void SaveForUndo (void)
{
SaveForUndo1(gUndo);
gRedo.clear();
fgLastWasUndo = false;
}

static void Undo (void)
{
if(NSIZE(gUndo))
    {
    QuickMsgBox("Undo");
    UNDO Undo(gUndo.back());
    gUndo.pop_back();
    SaveForUndo1(gRedo);
    SetLandmarkNbr(Undo.iLandmark);
    SetImgNbr(Undo.iImg);
    SetLandmarkPosition(Undo.iImg, Undo.iLandmark, Undo.x, Undo.y, true);
    fgForceRecenter = true;
    Redisplay();
    fgLastWasUndo = true;
    }
else if(fgLastWasUndo)
    QuickMsgBox("Nothing more to undo");
else
    QuickMsgBox("Nothing to undo");
}

static void Redo (void)
{
if(NSIZE(gRedo))
    {
    QuickMsgBox("Redo");
    UNDO Undo(gRedo.back());
    gRedo.pop_back();
    SaveForUndo1(gUndo);
    SetLandmarkNbr(Undo.iLandmark);
    SetImgNbr(Undo.iImg);
    SetLandmarkPosition(Undo.iImg, Undo.iLandmark, Undo.x, Undo.y, true);
    fgForceRecenter = true;
    Redisplay();
    fgLastWasUndo = false;
    }
else if(fgLastWasRedo)
    QuickMsgBox("Nothing more to redo");
else
    QuickMsgBox("Nothing to redo");
}

//-----------------------------------------------------------------------------
static void Revert (void)
{
// if reverting the current image, use undo to do the job so can redo the revert
const bool fUndoCurrentImg = NSIZE(gUndo) && gUndo[NSIZE(gUndo)-1].iImg == igImg;

if(fEqual(gShapes[igImg], gShapesOrg[igImg]))
    QuickMsgBox("No changes to this image (no undo needed)");
else if(fYesNoMsg(true,
            fUndoCurrentImg? "Undo all changes to the current image?\n\n"
                             "Note:  you can use redo to get your changes back":
                             "Undo all changes to the current image?\n\n"
                             "Note:  you cannot use redo to get your changes back"))
    {
    if(fUndoCurrentImg)
        {
        lprintf("UndoCurrentImg:\n");
        while(NSIZE(gUndo) && gUndo[NSIZE(gUndo)-1].iImg == igImg)
            {
            UNDO Undo(gUndo.back());
            gUndo.pop_back();
            SaveForUndo1(gRedo);
            SetLandmarkNbr(Undo.iLandmark);
            SetImgNbr(Undo.iImg);
            SetLandmarkPosition(Undo.iImg, Undo.iLandmark, Undo.x, Undo.y, true);
            }
        }
    else    // sadly, there is no undo if want to delete beyond the current image
        {
        gShapes[igImg] = gShapesOrg[igImg];
        gUndo.clear();
        gRedo.clear();
        }
    Redisplay();
    fgLastWasUndo = true;   // for Ctrl-Z
    TimedMsgBox("Undid all changes to this image");
    }
}

//-----------------------------------------------------------------------------
static void SetImgNbr (int iImg)
{
if(iImg != igImg)
    {
    ngPreloadOffset = iImg - igImg;
    igImg = iImg;
    if(igImg < 0)
        igImg = ngImgs - 1;
    else if(igImg >= ngImgs)
        igImg = 0;
    if(igCmdLineLandmark != -1) // user specified a landmark on cmd line?
        SetLandmarkNbr(igCmdLineLandmark);  // revert to that landmark
    igLandmark = CLAMP(igLandmark, 0, gShapes[igImg].rows-1); // needed if nlandmarks changed
    igLastClosestLandmark = -1;
    fgForceRecenter = true;
    }
}

//-----------------------------------------------------------------------------
static void SetLandmarkNbr (int iLandmark)
{
if(iLandmark != igLandmark)
    fgMustGetZoomedWidth = true;
igLandmark = iLandmark;
if(igLandmark < 0)
    igLandmark = gShapes[igImg].rows - 1;
else if(igLandmark >= gShapes[igImg].rows)
    igLandmark = 0;
if(iLandmark != igLandmark) // wrap around caused an additonal landmark change?
    fgMustGetZoomedWidth = true;
}

//-----------------------------------------------------------------------------
static void NotifyUserIfWrappedToFirstImg (void)
{
if(igImg == 0)
    {
    PlaySound(sgNotifySound, NULL, SND_ASYNC|SND_FILENAME|SND_NODEFAULT);
    if(!fgChangesSaved) // so don't put up box if check or view mode
        MsgBox("Back at first image");
    }
}

//-----------------------------------------------------------------------------
static void NextImg (int iMissingLandmarkJump = -1)
{
if(iMissingLandmarkJump < 0)
    SetImgNbr(igImg+1);
else  // jump to next image with this landmark missing
    {
    do
        {
        SetImgNbr(igImg+1);
        lprintf("igImg %d fPointUsed(igImg, iMissingLandmarkJump %d) %d\n",
            igImg, iMissingLandmarkJump, fPointUsed(igImg, iMissingLandmarkJump));
        }
    while(igImg != 0 && // not wrapped?
            fPointUsed(gShapes[igImg], iMissingLandmarkJump));
    }
NotifyUserIfWrappedToFirstImg();
}

//-----------------------------------------------------------------------------
static void PrevImg (void)
{
SetImgNbr(igImg-1);
}

//-----------------------------------------------------------------------------
// "example" becomes "example_L09xDATE_TIME"
//
// where
//   09 is the landmark number ("__" if no coords were changed)
//
//   x is "_" no other landmarks were changed
//        "x" if coords for other landmarks also changed

static const char *sGetBackupName (
    const char sPath[],
    const char sExt[],
    bool       fBackupDir)
{
static char sNewPath[SLEN];

char sDrive[_MAX_DRIVE], sDir[_MAX_DIR], sBase[_MAX_FNAME];
splitpath(sPath, sDrive, sDir, sBase, NULL);

static char sTime[SLEN];
_tzset(); // set time zone
time_t ltime; time(&ltime);
struct tm today; _localtime64_s(&today, &ltime );
strftime(sTime, SLEN, "%y%m%d_%H%M%S", &today);

char sLandmarkInfo[SLEN];
if(ngLandmarksUpdated)
    sprintf(sLandmarkInfo, "L%2.2d%s",
        igLandmark,
        fgMoreThanOneLandmarkNbrChanged? "x": "_");
else
    sprintf(sLandmarkInfo, "L___");

char sNewBase[SLEN];
sprintf(sNewBase, "%s_%s%s", sBase, sLandmarkInfo, sTime);
if(fBackupDir)
    {
    char sDir1[SLEN]; sprintf(sDir1, "backup/%s", sDir);
    makepath(sNewPath, sDrive, sDir1, sNewBase, sExt);
    }
else
    makepath(sNewPath, sDrive, sDir, sNewBase, sExt);
return sNewPath;
}

//-----------------------------------------------------------------------------
static void SaveState (void)
{
if(fgViewMode)  // for paranoia, should be redundant
    {
    MsgBox("Cannot save state in \"View\" mode");
    return;
    }
// write shapes to sgNewShapeFile

if(!fFileWriteable(sgNewShapeFile))
    MsgBox("Unable to write shape file:\n%s", sgNewShapeFile);
else
    {
    TimedMsgBox("Saving..."); // give immediate response to mouse click
    tprintf("Write %s\n", sgNewShapeFile);
    WriteShapeFile(sgNewShapeFile, gShapes, gTags, sgImgDirs);
    fgChangesSaved = true;
    }
// write backup shape file with timestamp in its name

const char *sShapeFile = sGetBackupName(sgNewShapeFile, "bshape", true);
if(!fFileWriteable(sShapeFile))
    {
    sShapeFile = sGetBackupName(sgNewShapeFile, "bshape", false);
    if(!fFileWriteable(sShapeFile))
        {
        MsgBox("Unable to write backup shape file:\n%s", sShapeFile);
        sShapeFile = NULL;
        }
    }
if(sShapeFile)
    {
    tprintf("Write %s\n", sShapeFile);
    WriteShapeFile(sShapeFile, gShapes, gTags, sgImgDirs);
    }
}

//-----------------------------------------------------------------------------
// This doesn't do all possible error checking, it doesn't matter, we
// use it only for backing up the log file.

static void CopyLogFile (const char sOld[], const char sNew[])
{
FILE *pOld = fopen(sOld,  "rt");
if(pOld)
    {
    FILE *pNew = fopen(sNew, "wt");
    if(pNew)
        {
        int c;
        while((c = fgetc(pOld)) != EOF)
            fputc(c, pNew);
        fputc(0x0d, pNew);  // carriage return
        fputc(0x0a, pNew);  // line feed
        fclose(pNew);
        }
    fclose(pOld);
    }
}

//-----------------------------------------------------------------------------
static bool fAPointWasMarked (void)
{
for(int iImg = 0; iImg < ngImgs; iImg++)
    for(int i = 0; i < gShapes[igImg].rows; i++)
        if(gMarked[iImg](i, IX) == MARKED)
            return true;
return false;
}

//-----------------------------------------------------------------------------
static bool fCheckSaveState (bool fAllowCancel) // return true if not cancel
{
int id = IDNO;
if(!fgChangesSaved)
    {
    if(fAllowCancel)
        id = nYesNoCancelMsg(true,
                "Save changes?\n\nWill save to %s\n\n",
                sgNewShapeFile);
    else if(fYesNoMsg(true,
                "Save changes?\n\nWill save to %s\n\n",
                sgNewShapeFile))
        {
        id = IDYES;
        }
    if(id == IDYES)
        SaveState();
    }
if(id != IDCANCEL && pgLogFile && fAPointWasMarked())
    {
    // copy marki.log to backup log file with timestamp in its name

    const char *sLogFile = sGetBackupName(sgNewShapeFile, "blog", true);
    if(!fFileWriteable(sLogFile))
        {
        sLogFile = sGetBackupName(sgNewShapeFile, "blog", false);
        if(!fFileWriteable(sLogFile))
            {
            MsgBox("Unable to write backup log file:\n%s", sLogFile);
            sLogFile = NULL;
            }
        }
    if(sLogFile)
        {
        tprintf("Copy %s to %s\n", sgMarkiLog, sLogFile);
        CopyLogFile(sgMarkiLog, sLogFile);
        }
    }
return id != IDCANCEL;
}

//-----------------------------------------------------------------------------
static void UpdateDlgWnd (void)
{
if(!hgDlgWnd)
    return;

CV_Assert(igImg >= 0);

char s[SLEN];
if(NSIZE(gTags))     // initialized?
    {
    ATTR uAttr; char sBase[SLEN]; DecomposeTag(&uAttr, sBase, gTags[igImg]);
    SetDlgItemText(hgDlgWnd, IDC_IMG_NAME, sBase);
    strcpy(s, sAttrAsString(uAttr, false, " "));
    // replace every third comma with \n because limited horizontal space
    int i = -1, nComma = 0;
    while(s[++i])
        if(s[i] == ',')
            {
            nComma++;
            if(nComma % 3 == 0)
                s[i] = '\n';
            }
#if 0
    if(gShapes[igImg].rows == 77)
        {
        if(fMouthOpen(gShapes[igImg]))
            strcat(s, "\n[Mouth open]");
        else
            strcat(s, "\n[Mouth closed]");
        }
#endif
    SetDlgItemText(hgDlgWnd, IDC_ATTR, s);
    }
SetWindowText(hgDlgWnd, ssprintf("Index %d", igImg)); // window header
SetDlgItemText(hgDlgWnd, IDC_IMG_NBR, ssprintf("%d", igImg));
SetDlgItemText(hgDlgWnd, IDC_NBR_IMGS,
    ssprintf("of %d   %g%%", ngImgs, (int)(1000 * (igImg + 1.) / ngImgs) / 10.));
SetDlgItemText(hgDlgWnd, IDC_LANDMARK, ssprintf("%d", igLandmark));
SetDlgItemText(hgDlgWnd, IDC_NBR_LANDMARKS, ssprintf("of %d", gShapes[igImg].rows));

char sSmooth[SLEN]; sSmooth[0] = 0;
if(fgSmoothInEffect)
    sprintf(sSmooth, "Sm %d ", ngSmoothAmount);

char sEqualize[SLEN]; sEqualize[0] = 0;
if(fgEqualize)
    sprintf(sEqualize, "Eq");

SetDlgItemText(hgDlgWnd, IDC_IMG_DIM,
    ssprintf(
        "%d x %d     %.1f MPixel  %s%s\nEye-mouth %.0f  ZWidth %.0f",
        gImg.cols, gImg.rows,
        gImg.cols * gImg.rows / (1024. * 1024.),
        sSmooth, sEqualize, gEyeMouthDist, gZoomedWidth));

char sLandmarkModified[SLEN]; sLandmarkModified[0] = 0;
if(gMarked[igImg](igLandmark, IX) == MARKED)
    sprintf(sLandmarkModified, "Landmark %d modified", igLandmark);

SetDlgItemText(hgDlgWnd, IDC_MODIFIED,
    ssprintf("%s\n%s", sLandmarkModified, (fgChangesSaved? "": "Changes not saved")));
}

//-----------------------------------------------------------------------------
static void DlgWmCommand (HWND hDlg, WPARAM wParam)
{
static bool fFirstTime = true;
static char sOldLandmark[SLEN];
static char sOldImgName[SLEN];
static char sOldImgNbr[SLEN];

if(fFirstTime)
    {
    // needed for fall back if a command below fails
    // e.g. the user enters an invalid landmark
    GetDlgItemText(hDlg, IDC_LANDMARK,   sOldLandmark,  SLEN);
    GetDlgItemText(hDlg, IDC_IMG_NAME, sOldImgName, SLEN);
    GetDlgItemText(hDlg, IDC_IMG_NBR,  sOldImgNbr,  SLEN);
    }

// Hack: make hitting return in an edit box the same as clicking on OK.
// This only works if ES_MULTILINE is used in marki.rc
// TODO clean this up, what is the proper way to do this?

#define isReturn(wParam) (((wParam >> 16) & 0xffff) == EN_MAXTEXT)

// TODO why doesn't hitting tab move you to the next control?

if(wParam == IDOK || isReturn(wParam))
    {
    char sLandmark[SLEN]; GetDlgItemText(hDlg, IDC_LANDMARK, sLandmark, SLEN);
    if(sLandmark[0] == 0)
        strcpy(sLandmark, sOldLandmark);

    char sImgName[SLEN]; GetDlgItemText(hDlg, IDC_IMG_NAME, sImgName, SLEN);
    if(sImgName[0] == 0)
        strcpy(sImgName, sOldImgName);

    char sImgNbr[SLEN]; GetDlgItemText(hDlg, IDC_IMG_NBR, sImgNbr, SLEN);
    if(sImgNbr[0] == 0)
        strcpy(sImgNbr, sOldImgNbr);

    if(fFirstTime || strcmp(sImgName, sOldImgName)) // image name changed?
        {
        if(sImgName[0] == 0)
            {
            SetDlgItemText(hDlg, IDC_IMG_NAME, sOldImgName);
            SetDlgItemText(hDlg, IDC_IMG_NBR,  sOldImgNbr);
            }
        else
            {
            int iImg = igImg, iCount;
            for(iCount = 0; iCount < ngImgs; iCount++)
                {
                iImg = iImg + 1;
                if(iImg == ngImgs)
                    iImg = 0;
                // use strstr so match if sImgName appears anywhere in the basename
                if(strstr(sGetBaseFromTag(gTags[iImg]), sImgName))
                    break;
                }
            if(iCount == ngImgs) // did not find it
                {
                TimedMsgBox("No image name includes \"%s\"", sImgName);
                SetDlgItemText(hDlg, IDC_IMG_NAME, sOldImgName);
                SetDlgItemText(hDlg, IDC_IMG_NBR,  sOldImgNbr);
                }
            else if(iImg != igImg)
                {
                SaveForUndo();
                SetImgNbr(iImg);
                sprintf(sImgNbr, "%d", igImg);
                SetDlgItemText(hDlg, IDC_IMG_NBR,  sImgNbr);
                SetDlgItemText(hDlg, IDC_IMG_NAME, sGetCurrentBase());
                }
            }
        }
    if(fFirstTime || strcmp(sImgNbr, sOldImgNbr)) // image nbr changed?
        {
        int iInput = -1;
        if(1 != sscanf(sImgNbr, "%d", &iInput) || iInput < 0 || iInput >= (int)ngImgs)
            {
            TimedMsgBox("Bad image number \"%s\", allowed range is 0 to %d",
                         sImgNbr, ngImgs-1);
            SetDlgItemText(hDlg, IDC_IMG_NAME, sOldImgName);
            SetDlgItemText(hDlg, IDC_IMG_NBR,  sOldImgNbr);
            }
        else if(iInput != igImg)
            {
            SaveForUndo();
            SetImgNbr(iInput);
            sprintf(sImgNbr, "%d", igImg);
            SetDlgItemText(hDlg, IDC_IMG_NBR,  sImgNbr);
            SetDlgItemText(hDlg, IDC_IMG_NAME, sGetCurrentBase());
            }
        }
    if(fFirstTime || strcmp(sLandmark, sOldLandmark)) // landmark nbr changed?
        {
        int iInput = -1;
        if(1 != sscanf(sLandmark, "%d", &iInput) ||
                    iInput < 0 || iInput >= gShapes[igImg].rows)
            {
            TimedMsgBox("Bad landmark number \"%s\", allowed range is 0 to %d",
                        sLandmark, gShapes[igImg].rows-1);
            SetDlgItemText(hDlg, IDC_LANDMARK,  sOldLandmark);
            }
        else if(iInput != igLandmark)
            {
            SaveForUndo();
            SetLandmarkNbr(iInput);
            sprintf(sLandmark, "%d", igLandmark);
            SetDlgItemText(hDlg, IDC_LANDMARK,  sLandmark);
            }
        }
    GetDlgItemText(hDlg, IDC_LANDMARK,   sOldLandmark,  SLEN);
    GetDlgItemText(hDlg, IDC_IMG_NAME, sOldImgName, SLEN);
    GetDlgItemText(hDlg, IDC_IMG_NBR,  sOldImgNbr,  SLEN);
    Redisplay();
    SetFocus(hgChildWnd);   // set focus back to child
    fFirstTime = false;
    }
}

//-----------------------------------------------------------------------------
INT_PTR CALLBACK DlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
switch(msg)
    {
    case WM_INITDIALOG:
        {
        // Move the dialog window to the right place.  It's actually a
        // top level window so we need to position wrt the screen
        RECT RectWorkArea;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &RectWorkArea, 0);
        MoveWindow(hDlg, xgDlg, ygDlg, ngDlgWidth, ngDlgHeight, true);
        UpdateDlgWnd();
        return 0;
        }
    case WM_COMMAND:
        DlgWmCommand(hDlg, wParam);
        return 0;
    case WM_CLOSE:
        if(IsIconic(hDlg))                  // minimized?
            ShowWindow(hDlg, SW_RESTORE);   // then unminimize it
        else
            ShowWindow(hDlg, SW_MINIMIZE);  // else minimize it
        return 0;   // do not invoke the default window close handler
    }
return 0;
}

//-----------------------------------------------------------------------------
// tell the buttons to display themselves as depressed or not

static void DisplayButtons (void)
{
DisplayButton(hgToolbar, IDM_Lock,           fgLock);
DisplayButton(hgToolbar, IDM_Gray,           fgGray);
DisplayButton(hgToolbar, IDM_Dark,           fgDark);
DisplayButton(hgToolbar, IDM_ConnectDots,    fgConnectDots);
DisplayButton(hgToolbar, IDM_ShowNbrs,       fgShowNbrs);
DisplayButton(hgToolbar, IDM_ShowActiveNbr,  fgShowActiveNbr);
DisplayButton(hgToolbar, IDM_ShowAllDots,    fgShowAllDots);
DisplayButton(hgToolbar, IDM_Center,         fgCenter);
DisplayButton(hgToolbar, IDM_AutoNextImg,    fgAutoNextImg);
DisplayButton(hgToolbar, IDM_JumpToLandmark, fgJumpToLandmark);
// DisplayButton(hgToolbar, IDM_AutoNextPoint,  fgAutoNextPoint);
}

//-----------------------------------------------------------------------------
static void IdmGray (void)
{
fgGray ^= 1;
int w2 = gImg.cols / 2, h2 = gImg.rows / 2;
CV_Assert(gImg.cols > 3 && gImg.rows > 3);
if(fgGray &&
      gImg(0,0)[0]   == gImg(0,0)[1]   && gImg(0,0)[1]   == gImg(0,0)[2] &&
      gImg(1,3)[0]   == gImg(1,3)[1]   && gImg(1,3)[1]   == gImg(1,3)[2] &&
      gImg(w2,h2)[0] == gImg(w2,h2)[1] && gImg(w2,h2)[1] == gImg(w2,h2)[2])
    {
    TimedMsgBox("This image is already monochrome?");
    }
SetChildCursor();
}

//-----------------------------------------------------------------------------
static void IdmDark (void)
{
fgDark ^= 1;
}

//-----------------------------------------------------------------------------
static void IdmShowActiveNbr (void)
{
if(fgJumpToLandmark)
    TimedMsgBox("Marki will always show the active landmark\n"
                "because you have chosen auto-jump-to-landmark");
else
    fgShowActiveNbr ^= 1;
}

//-----------------------------------------------------------------------------
static void IdmShowLandmarks (void)
{
if(fgSpecialPoints)
    QuickMsgBox("That command is not available in \"Special Points\" mode");
else
    fgShowAllDots ^= 1;
}

//-----------------------------------------------------------------------------
static void IdmConnectDots (void)
{
if(fgSpecialPoints)
    QuickMsgBox("That command is not available in \"Special Points\" mode");
else
    fgConnectDots ^= 1;
}

//-----------------------------------------------------------------------------
static void IdmShowNbrs (void)
{
if(fgSpecialPoints)
    QuickMsgBox("That command is not available in \"Special Points\" mode");
else
    fgShowNbrs ^= 1;
}

//-----------------------------------------------------------------------------
static void IdmCenter (void)
{
if(fFaceDetMat())
    QuickMsgBox("Cannot center a face detector point");
else
    {
    fgCenter ^= 1;
    fgMustGetZoomedWidth = true;
    fgForceRecenter = true;
    }
}

//-----------------------------------------------------------------------------
static void IdmZoomOut (void)
{
if(fFaceDetMat())
    QuickMsgBox("Cannot center a face detector point");
else
    {
    if(!fgCenter)
        fgCenter = true;
    else if(gZoomAmount < MIN_ZOOM + .0001) // .0001 takes care of rounding
        {
        gZoomAmount = MIN_ZOOM; // clamp
        QuickMsgBox("Minimum zoom");
        }
    else
        gZoomAmount /= ZOOM_RATIO;
    fgMustGetZoomedWidth = true;
    fgForceRecenter = true;
    }
}

//-----------------------------------------------------------------------------
static void IdmZoomIn (void)
{
if(fFaceDetMat())
    QuickMsgBox("Cannot center a face detector point");
else
    {
    if(!fgCenter)
        fgCenter = true;
    else if(gZoomAmount < MIN_ZOOM)
        gZoomAmount = MIN_ZOOM;
    else
        {
        gZoomAmount *= ZOOM_RATIO;
        if(gZoomAmount > MAX_ZOOM - .0001) // .0001 takes care of rounding
            {
            gZoomAmount = MAX_ZOOM;
            QuickMsgBox("Maximum zoom");
            }
        }
    fgMustGetZoomedWidth = true;
    fgForceRecenter = true;
    }
}

//-----------------------------------------------------------------------------
static void IdmHelp (void)
{
UnownedMsgBox(
    "Marki Help", "%s\n%s version %s%s      www.milbo.users.sonic.net\n",
    sgHelp, sgAppName, STASM_VERSION, sgVersionAppend);
}

//-----------------------------------------------------------------------------
static void SkipToPrevSpecialPoint (void)
{
SetLandmarkNbr(igLandmark - 1);
while(!gSpecialPoints(igLandmark))
    if(--igLandmark == 0)
        igLandmark = gShapes[igImg].rows - 1;
}

//-----------------------------------------------------------------------------
static void SkipToNextSpecialPoint (void)
{
SetLandmarkNbr(igLandmark + 1);
while(!gSpecialPoints(igLandmark))
    if(++igLandmark == gShapes[igImg].rows)
        igLandmark = 0;
}

//-----------------------------------------------------------------------------
static void IdmPrevPoint (int nRepeats, bool fSelectableLandmarksOnly)
{
if(fFaceDetMat())
    QuickMsgBox("Cannot change a face detector shape");
else
    {
    fgJumpToLandmark = false;
    ReleaseCapture();
    igLastClosestLandmark = -1;
    SaveForUndo();
    if(fgSpecialPoints)
        SkipToPrevSpecialPoint();
    else if(fgBigH && fSelectableLandmarksOnly)
        {
        // skip to the previous "Selectable" point (set by -H flag)
        int iLandmark = igLandmark;
        int iSafety = ngMaxNbrLandmarks + 1;
        do
            {
            if(--iLandmark < 0)
                iLandmark = gShapes[igImg].rows - 1;
            }
        while(!fgBigHLandmarks[iLandmark] && iSafety--);
        SetLandmarkNbr(iLandmark);
        }
    else for(int iRepeat = 0; iRepeat < nRepeats; iRepeat++)
        SetLandmarkNbr(igLandmark - 1);
    }
}

//-----------------------------------------------------------------------------
static void IdmNextPoint (int nRepeats, bool fSelectableLandmarksOnly)
{
if(fFaceDetMat())
    QuickMsgBox("Cannot change a face detector shape");
else
    {
    fgJumpToLandmark = false;
    ReleaseCapture();
    igLastClosestLandmark = -1;
    SaveForUndo();
    if(fgSpecialPoints)
        SkipToNextSpecialPoint();
    else if(fgBigH && fSelectableLandmarksOnly)
        {
        // skip to the next Selectable point (set by -H flag)
        int iLandmark = igLandmark;
        int iSafety = ngMaxNbrLandmarks + 1;
        do
            {
            if(++iLandmark >= gShapes[igImg].rows)
                iLandmark = 0;
            }
        while(!fgBigHLandmarks[iLandmark] && iSafety--);
        SetLandmarkNbr(iLandmark);
        }
    else for(int iRepeat = 0; iRepeat < nRepeats; iRepeat++)
        SetLandmarkNbr(igLandmark + 1);
    }
}

//-----------------------------------------------------------------------------
static void MarkPointAsUnused (void)
{
if(fgViewMode)
    QuickMsgBox("Landmarks locked in \"View\" mode");
else if(fgSpecialPoints)
    QuickMsgBox("Landmarks locked in \"Special Points\" mode");
else if(fgLock)
    TimedMsgBox("Landmarks locked\n\nClick the Unlock button to unlock them");
else
    {
    QuickMsgBox("Point deleted");
    if(fPointUsed(gShapes[igImg], igLandmark))
        {
        SaveForUndo();
        SetLandmarkPosition(igImg, igLandmark, 0, 0, true);
        }
    gMarked[igImg](igLandmark, IX) = MFLOAT(MARKED);
    if(fgAutoNextImg)
        NextImg();
    else if(fgAutoNextPoint)
        SetLandmarkNbr(igLandmark+1);
    }
}

//-----------------------------------------------------------------------------
static void ForwardToNextTenPercentBoundary (void)
{
int iImg = igImg;
if(iImg == ngImgs - 1)      // if last image, then roll over to fist image
    iImg = 0;
else
    {
    int n10 = MAX((int)(ngImgs / 10.), 1);
    iImg = (((iImg+1) / n10) + 1) * n10 - 1; // -1 because we use 0 based indexing
    if(iImg >= ngImgs)
        iImg = ngImgs - 1; // clamp at last image
    }
SetImgNbr(iImg);
}

//-----------------------------------------------------------------------------
static void BackwardToNextTenPercentBoundary (void)
{
int iImg = igImg;
if(iImg == 0)               // if first image, then roll over to last image
    iImg = ngImgs - 1;
else
    {
    int n10 = MAX((int)(ngImgs / 10.), 1);
    iImg = ((iImg-1) / n10) * n10 - 1;
    if(iImg < 0)
        iImg = 0;       // clamp at first image
    }
SetImgNbr(iImg);
}

//-----------------------------------------------------------------------------
static bool fCheckCheckMode (void)
{
if(fgCheckMode)
    QuickMsgBox("That command is not allowed in check mode");
return fgCheckMode;
}

//-----------------------------------------------------------------------------
// a toolbar button was pressed

static void Wm_Command (HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
ClosedTimedMsgBox();
fgGoBackOnEsc = true;
fgIssueEscMsg = false;
const int nRepeats = (fgShiftKeyDown? 10: 1);
int iRepeat;

switch(wParam)
    {
    case IDM_Save:
        if(fCheckCheckMode())
            break;
        if(fgViewMode)
            TimedMsgBox("\"Save landmarks\" is not allowed in \"View\" mode");
        else if(fgSpecialPoints)
            TimedMsgBox("\"Save landmarks\" is not allowed in \"Special Points\" mode");
        else
            {
            SaveState();
            if(fgChangesSaved)
                TimedMsgBox("Saved to %s", sgNewShapeFile);
            UpdateDlgWnd();
            }
        break;

    case IDM_Lock:
        if(fCheckCheckMode())
            break;
        if(fgViewMode)
            QuickMsgBox("Cannot unlock in \"View\" mode");
        else if(fgSpecialPoints)
            TimedMsgBox("Cannot unlock in \"Special Points\" mode");
        else if(fgLock)
            {
            fgLock = 0;
            QuickMsgBox("Unlocked");
            }
        else
            {
            fgLock = 1;
            QuickMsgBox("Locked");
            }
        break;

    case IDM_Undo:
        if(fCheckCheckMode())
            break;
        Undo();
        break;

    case IDM_Redo:
        if(fCheckCheckMode())
            break;
        Redo();
        break;

    case IDM_Revert:
        if(fCheckCheckMode())
            break;
        Revert();
        break;

    case IDM_Gray:
        IdmGray();
        break;

    case IDM_Dark:
        IdmDark();
        break;

    case IDM_ShowActiveNbr:
        IdmShowActiveNbr();
        break;

    case IDM_ShowAllDots:
        IdmShowLandmarks();
        break;

    case IDM_ConnectDots:
        IdmConnectDots();
        break;

    case IDM_ShowNbrs:
        IdmShowNbrs();
        break;

    case IDM_WriteImg:
        if(fCheckCheckMode())
            break;
        IdmWriteImg(hgChildWnd, sGetCurrentBase());
        break;

    case IDM_Center:
        IdmCenter();
        break;

    case IDM_ZoomOut:
        IdmZoomOut();
        break;

    case IDM_ZoomIn:
        IdmZoomIn();
        break;

    case IDM_PrevImg:
        if(fCheckCheckMode())
            break;
        SaveForUndo();
        if(fgCtrlKeyDown)        // go forward to next 10% boundary
            BackwardToNextTenPercentBoundary();
        else
            {
            for(iRepeat = 0; iRepeat < nRepeats; iRepeat++)
                PrevImg();
            ngPreloadOffset = -nRepeats;
            }
        break;

    case IDM_NextImg:
        if(fCheckCheckMode())
            break;
        SaveForUndo();
        if(fgCtrlKeyDown)        // go forward to next 10% boundary
            ForwardToNextTenPercentBoundary();
        else
            {
            for(iRepeat = 0; iRepeat < nRepeats; iRepeat++)
                NextImg();
            ngPreloadOffset = nRepeats;
            }

        break;

    case IDM_AutoNextImg:
        if(fCheckCheckMode())
            break;
        fgAutoNextImg ^= 1;
        if(fgAutoNextImg)
            fgAutoNextPoint = false;
        break;

    case IDM_JumpToLandmark:
        if(fCheckCheckMode())
            break;
        fgJumpToLandmark ^= 1;
        igLastClosestLandmark = -1;
        fgAutoNextImg = fgAutoNextPoint = false;
        if(fgJumpToLandmark)
            {
            fgShowActiveNbr = 1;
            SetCapture(hgChildWnd);
            }
        else
            ReleaseCapture();
        break;

    case IDM_AutoNextPoint:
        if(fCheckCheckMode())
            break;
        fgAutoNextPoint ^= 1;
        fgJumpToLandmark = false;
        ReleaseCapture();
        igLastClosestLandmark = -1;
        if(fgAutoNextPoint)
            fgAutoNextImg = false;
        break;

    case IDM_PrevPoint:
        if(fCheckCheckMode())
            break;
        IdmPrevPoint(nRepeats, true);
        break;

    case IDM_NextPoint:
        if(fCheckCheckMode())
            break;
        IdmNextPoint(nRepeats, true);
        break;

    case IDM_Help:
        if(fCheckCheckMode())
            break;
        IdmHelp();
        break;

    case IDM_Blank:
        break;

    default:
        Err1("Wm_Command bad param %u", wParam);
        break;
    }
fgLastWasRedo = (wParam == IDM_Redo);
if(!fgCheckMode &&
        (wParam != IDM_Blank && wParam != IDM_Undo &&
         wParam != IDM_Redo && wParam != IDM_Revert))
    {
    Redisplay();
    }
}

//-----------------------------------------------------------------------------
static void SetShowMetaData (unsigned uShow)
{
if(ugShowDetData == uShow)
    ugShowDetData = 0;
else
    {
    ugShowDetData = uShow;
    if(!fgShowPoseData) // first time?
        QuickMsgBox("Getting meta data");
    fgShowPoseData = true;
    }
}

//-----------------------------------------------------------------------------
static void Wm_Char (HWND hWnd, WPARAM wParam)
{
ClosedTimedMsgBox();
bool fRedisplay = true;
switch(wParam)
    {
    case 'A':   // IDM_Gray
    case 'a':
        IdmGray();
        break;
    case 'S':   // IDM_Dark
    case 's':
        IdmDark();
        break;
    case 'D':   // IDM_ShowActiveNbr
    case 'd':
        IdmShowActiveNbr();
        break;
    case 'F':   // IDM_ShowAllDots
    case 'f':
        IdmShowLandmarks();
        break;
    case 'G':   // IDM_ConnectDots
    case 'g':
        IdmConnectDots();
        break;
    case 'H':   // IDM_ShowNbrs
    case 'h':
        IdmShowNbrs();
        break;
    case 'Z':   // IDM_Center
    case 'z':
        IdmCenter();
        break;
    case 'X':   // IDM_ZoomOut
    case 'x':
        IdmZoomOut();
        break;
    case 'C':   // IDM_ZoomIn
    case 'c':
        IdmZoomIn();
        break;
    case 'V':   // zoom out 2 levels
    case 'v':
        IdmZoomOut();
        IdmZoomOut();
        break;
    case 'B':   // zoom in 2 levels
    case 'b':
        IdmZoomIn();
        IdmZoomIn();
        break;
    case '\t':                   // mark point as unused
        MarkPointAsUnused();
        break;
    // TODO commented out, causes crash in EqualizeImgUnderShape
    // case 'Q':
    // case 'q':
    //     fgEqualize ^= 1;
    //     logprintf("Equalize %d\n", fgEqualize);
    //     break;
    case 'W':
    case 'w':
        ngSmoothAmount = (ngSmoothAmount == 1)? 2: 1;
        logprintf("Smooth %d\n", ngSmoothAmount);
        break;
    case '.':
        fgSmallPoints ^= 1;
        break;
    case '?':
        IdmHelp();
        break;
    case 0x03:                  // Ctrl-C
        tprintf("Ctrl-C\n");
        PostMessage(hWnd, WM_CLOSE, 0, 0); // self destroy
        break;
        break;
    case 0x0d:                  // allow user to put blank lines in command window
        lprintf("\n");
        break;
    case 0x0e:                  // Ctrl-N
        fgShowActiveNbr = true;
        IdmNextPoint(fgShiftKeyDown? 10: 1, false);
        break;
    case 0x10:                  // Ctrl-P
        fgShowActiveNbr = true;
        IdmPrevPoint(fgShiftKeyDown? 10: 1, false);
        break;
    case 0x11:                  // Ctrl-Q, all metadata
        SetShowMetaData(AT_Meta); // use AT_Meta to mean all det data
        break;
    case 23:                    // Ctrl-W
        SetShowMetaData(AT_DetYaw00);
        break;
    case 5:                     // Ctrl-E
        SetShowMetaData(AT_DetYaw22|AT_DetYaw_22);
        break;
    case 18:                    // Ctrl-R
        SetShowMetaData(AT_DetYaw45|AT_DetYaw_45);
        break;
    case 0x14:                  // Ctrl-T
        fgTrace ^= 1;
        tprintf("Trace %d\n", fgTrace);
        break;
    case 0x1a:
        if(fgLastWasUndo)       // Ctrl-Z
            {
            Redo();
            fgLastWasRedo = true;
            }
        else
            {
            Undo();
            fgLastWasRedo = false;
            }
        fRedisplay = false;     // Undo/Redo have already done Redisplay for us
        break;
    case 0x1b:                  // ESC
        if(!fgCheckMode)
            {
            if(fgGoBackOnEsc)
                {
                SetImgNbr(igImg-1);
                fgGoBackOnEsc = false;
                }
            else
                {
                SetImgNbr(igImg+1);
                fgGoBackOnEsc = true;
                }
            fgIssueEscMsg = true;
            }
        break;
    default:                    // unrecognized character, ignore it
        fRedisplay = false;
        break;
    }
if(wParam != 0x1a)  // Ctrl-Z
    fgLastWasRedo = false;
if(fRedisplay && !fgCheckMode)
    Redisplay();
}

//-----------------------------------------------------------------------------
static void Wm_Keydown (HWND hWnd, WPARAM wParam)
{
ClosedTimedMsgBox();
fgPrepareRawImg = false;
bool fRedisplay = true;
const int nRepeats = (fgShiftKeyDown? 10: 1);
int iRepeat;

switch(wParam)
    {
    case VK_SHIFT:             // shift key
        fgShiftKeyDown = true;
        fRedisplay = false;
        fgGoBackOnEsc = true;
        fgIssueEscMsg = false;
        break;
    case VK_CONTROL:           // control
        fgCtrlKeyDown = true;
        fRedisplay = false;
        fgGoBackOnEsc = true;
        fgIssueEscMsg = false;
        break;
    case ' ':                   // space
        if(fgCheckMode)
            {
            fgPauseCheck ^= 1;
            Redisplay();
            break;
            }
        else if(fgCtrlKeyDown)
            {
            // control-space is page up
            SaveForUndo();
            PrevImg();
            break;
            }
        // FALL THROUGH!
    case VK_NEXT:               // page down
    case VK_DOWN:               // down arrow
    case VK_RIGHT:              // right arrow
        SaveForUndo();
        if(ugTagAttr && wParam == VK_RIGHT)
            {
            TagCurrentShape(true);
            NextImg();
            }
        else if(fgCtrlKeyDown)  // go forward to next 10% boundary
            ForwardToNextTenPercentBoundary();
        else
            {
            for(iRepeat = 0; iRepeat < nRepeats; iRepeat++)
                NextImg();
            ngPreloadOffset = nRepeats;
            }
        fgGoBackOnEsc = true;
        fgIssueEscMsg = false;
        break;
    case VK_PRIOR:              // page up
    case VK_UP:                 // up arrow
    case VK_LEFT:               // left arrow
        SaveForUndo();
        if(ugTagAttr && wParam == VK_LEFT)
            {
            TagCurrentShape(false);
            NextImg();
            }
        else if(fgCtrlKeyDown)  // go back to previous 10% boundary
            BackwardToNextTenPercentBoundary();
        else
            {
            for(iRepeat = 0; iRepeat < nRepeats; iRepeat++)
                PrevImg();
            ngPreloadOffset = -nRepeats;
            }
        fgGoBackOnEsc = true;
        fgIssueEscMsg = false;
        break;
    case VK_HOME:               // goto first image
        SaveForUndo();
        SetImgNbr(0);
        fgGoBackOnEsc = true;
        fgIssueEscMsg = false;
        break;
    case VK_END:                // goto last image
        SaveForUndo();
        SetImgNbr(ngImgs-1);
        fgGoBackOnEsc = true;
        fgIssueEscMsg = false;
        break;
    case VK_F1:
        IdmHelp();
        fgGoBackOnEsc = true;
        fgIssueEscMsg = false;
        break;
    case 0x1b:              // ESC
        if(fgCheckMode)
            {
            fgCheckMode = false;
            MsgBox("Check mode cancelled by ESC");
            Redisplay();
            }
        break;
    default:                    // unrecognized key, ignore it
        fRedisplay = false;
    }
if(fRedisplay && !fgCheckMode)
    Redisplay();
}

//-----------------------------------------------------------------------------
static const char *
sFormatElapsedTime (double ElapsedTime)
{
int temp1, temp;
static char s[SLEN];

if(ElapsedTime >= (60.0 * 60.0 * CLOCKS_PER_SEC))
    {
    temp = (int)(ElapsedTime / (60.0 * CLOCKS_PER_SEC));
    temp1 = temp / 60;
    sprintf(s, "%d hour%s %d min", temp1, sPluralize(temp1),
        temp - 60 * temp1);
    }
else if(ElapsedTime >= 60.0 * CLOCKS_PER_SEC)
    {
    temp1 = (int)(ElapsedTime / (60.0 * CLOCKS_PER_SEC));
    temp = (int)(ElapsedTime/CLOCKS_PER_SEC - 60 * temp1);
    sprintf(s, "%d min %d sec%s", temp1, temp, sPluralize(temp));
    }
else
    sprintf(s, "%.2f secs", ElapsedTime / CLOCKS_PER_SEC);

return s;
}

//-----------------------------------------------------------------------------
static void IssueElapsedTimeMsg (void)
{
double ElapsedTime = clock() - gStartTime;
tprintf("Current image %s is at index %d\n", sGetCurrentBase(), igImg);

if(!fgCheckMode)
    logprintf("%d landmarks updated in %d seconds\n",
        ngLandmarksUpdated, int(ElapsedTime / CLOCKS_PER_SEC));

tprintf("%s elapsed.\n", sFormatElapsedTime(ElapsedTime));
}

//-----------------------------------------------------------------------------
static void CheckMode_NextImg (void)
{
if(fgCheckMode && !fgPauseCheck)
    {
    int iImg = igImg + 1;
    if(iImg == ngImgs)
        {
        SetImgNbr(0);
        NotifyUserIfWrappedToFirstImg();
        IssueElapsedTimeMsg();
        IssueCheckModeResults();
        fgCheckMode = fgPauseCheck = false;
        PostMessage(hgMainWnd, WM_CLOSE, 0, 0); // self destroy
        }
    else
        {
        SetImgNbr(iImg);
        DisplayButtons();   // TODO in check mode a pushed button stays pushed, why?
        LoadCurrentImg();
        DisplayTitle(false);
        // trigger WM_PAINT, which will reinvoke CheckMode_NextImg
        InvalidateRect(hgChildWnd, NULL, false);
        }
    }
}

//-----------------------------------------------------------------------------
static void IssueCheckModeResults (void)
{
char sWriteable[SLEN]; sWriteable[0] = 0;
if(!fFileWriteable(sgNewShapeFile))
    sprintf(sWriteable,
        "\n\nNote that %s is not writeable", sgNewShapeFile);

lprintf("\n");
if(ngBadImgs == 0 && ngBadShapes == 0 && ngBadEyeMouth == 0)
    {
    if(igFirstImg == 0)
        MsgBox("All images checked, all ok%s", sWriteable);
    else
        MsgBox("%d images checked, all ok\n\n"
            "Note: the check started at %s, "
            "not the first image%s\n\n"
            "To check all images:  restart marki -C\n"
            "To always check all images, use marki -F -C\n",
            ngImgs - igFirstImg, sGetFirstBase(), sWriteable);
    }
else
    {
    char sFirstBad[SLEN]; sFirstBad[0] = 0;
    if(ngBadImgs)
        sprintf(sFirstBad, "\n\nFirst bad image:  %s",
                sgFirstBadImg);

    char sFirstBadShape[SLEN]; sFirstBadShape[0] = 0;
    if(ngBadShapes)
        sprintf(sFirstBadShape, "\n\nFirst bad shape:  %s",
                sgFirstBadShape);

    char sBadEyeMouthDist[SLEN];
    if(ngBadEyeMouth)
         sprintf(sBadEyeMouthDist,
            "\n\nCould not estimate the eye-mouth distance of %d image%s",
            ngBadEyeMouth, sPluralize(ngBadEyeMouth));
    else
        sprintf(sBadEyeMouthDist,
            "\n\nWas able to estimate the eye-mouth distance on all loaded images");

    if(igFirstImg == 0)
        MsgBox("All images checked (%d image%s could not be loaded)%s%s%s%s",
            ngBadImgs, sPluralize(ngBadImgs),
            sFirstBad, sFirstBadShape, sBadEyeMouthDist, sWriteable);
    else
        MsgBox("%d images checked (%d image%s could not be loaded)%s%s\n\n"
            "Note: the check started at %s, "
            "not the first image\n\n"
            "To check all images:  restart marki -C\n"
            "To always check all images, use marki -F -C%s\n",
            ngImgs - igFirstImg, ngBadImgs, sPluralize(ngBadImgs),
            sFirstBad, sBadEyeMouthDist, sGetFirstBase(), sWriteable);
    }
lprintf("\n");
}

//-----------------------------------------------------------------------------
static void DisplayCheckModeTitle (const char sShapeType[],
                                   const char sZoom[])
{
if(ngBadImgs)
    SetWindowText(hgMainWnd,
        ssprintf(
            "%s %5.1f%%%s   %s%d image%s could not be loaded%s   Started at %s    "
            "(First bad image is %s)",
            fgPauseCheck? "CHECK PAUSED": "CHECK",
            (int)(1000 * (igImg + 1.) / ngImgs) / 10.,
            sShapeType,
            sZoom,
            ngBadImgs, sPluralize(ngBadImgs),
            ngBadEyeMouth? " and some eye-mouth dist problems": "",
            igFirstImg == 0? "the first image": sGetFirstBase(),
            sgFirstBadImg));
else
    SetWindowText(hgMainWnd,
        ssprintf(
            "%s %5.1f%%%s   %sAll images ok so far%s   Started at %s",
            fgPauseCheck? "CHECK PAUSED": "CHECK",
            (int)(1000 * (igImg + 1.) / ngImgs) / 10.,
            sShapeType,
            sZoom,
            ngBadEyeMouth? " but some eye-mouth dist problems": "",
            igFirstImg == 0? "the first image": sGetFirstBase()));

}

//-----------------------------------------------------------------------------
static void DisplayStandardTitle (bool fWmMouseMove,
                                  const char sShapeType[],
                                  const char sZoom[])
{
char sView[SLEN]; sView[0] = 0;
if(fgViewMode)
    sprintf(sView, " VIEW");
else if(fgSpecialPoints)
    sprintf(sView, " COMPARE MARKERS");
else if(ugTagAttr)
    sprintf(sView, " TAG %x", ugTagAttr);

char sCenter[SLEN]; sCenter[0] = 0;
if(igCenterLandmark >= 0)
    sprintf(sCenter, "Center on %d " , igCenterLandmark);

char sPose[SLEN]; sPose[0] = 0;
if(fgShowPoseData)
    {
    const VEC Pose = GetPoseFromShapeFile(sGetCurrentBase(), sgShapeFile);
    if(NTOTAL(Pose))
        sprintf(sPose, "    yaw %.1f   pitch %.1f   roll %.1f   err %-.3f",
            Pose(0), Pose(1), Pose(2), Pose.cols>=4? Pose(3): 0.);
    else
        sprintf(sPose, "    No pose data");
    }
char s[SLEN];
// if(fgTrace) // show mouse coords in window title?
if(1) // TODO always show mouse coords
    {
    MFLOAT xMouse, yMouse;
    static MFLOAT xMouseOld, yMouseOld;
    if(fWmMouseMove)       // invoked by Wm_MouseMove?
        {                   // is so, use latest mouse position
        xMouse = xgMouse;
        yMouse = ygMouse;
        }
    else
        {                   // else use saved mouse positions
        xMouse = xMouseOld;
        yMouse = yMouseOld;
        }
    xMouseOld = xMouse;
    yMouseOld = yMouse;
    MouseToImgCoords(xMouse, yMouse, xMouse, yMouse);
    sprintf(s, "%s%s%s   %s      %8.0f %8.0f         %s%s%s",
            sGetCurrentBase(), sShapeType, sView, sCenter,
            xMouse, yMouse,
            sZoom, sgShapeFile, sPose);
    }
else // standard display
    sprintf(s, "%s %s%s   %s         %s%s%s",
            sView, sGetCurrentBase(), sShapeType, sCenter,
            sZoom, sgShapeFile, sPose);

SetWindowText(hgMainWnd, s);
}

//-----------------------------------------------------------------------------
static void DisplayTitle (bool fWmMouseMove)
{
char sZoom[SLEN]; sZoom[0] = 0;
if(fgCenter)
    sprintf(sZoom, "zoom %.1f    ", gZoomAmount);
char *sShapeType = "";
if(fFaceDetMat())
    sShapeType = " FACEDET";

if(fgCheckMode)
    DisplayCheckModeTitle(sShapeType, sZoom);
else
    DisplayStandardTitle(fWmMouseMove, sShapeType, sZoom);
}

//-----------------------------------------------------------------------------
static void FillRect (HDC hdc, HBRUSH hBrush,
                      MFLOAT left, MFLOAT top, MFLOAT right, MFLOAT bottom)
{
RECT rect;
rect.left   = cvRound(left);
rect.top    = cvRound(top);
rect.right  = cvRound(right);
rect.bottom = cvRound(bottom);
FillRect(hdc, &rect, hBrush);
}

//-----------------------------------------------------------------------------
// StretchDIBits fails if the image width is not divisible by 4.
// This is an (expensive) work around: pad the right side of the image so the
// width is divisible by 4.  Note that this doesn't mess up the mouse coords
// i.e. we still get the correct coords from the mouse and update the
// correct x location in the image.

static RGBV *
pGetImg4 (int   &nWidth4,   // out: the new image width (often same as original)
          const CIMAGE Img) // in
{
RGBV *pData = (RGBV *)Img.data;
nWidth4 = Img.cols;
if(nWidth4 & 3) // not divisible by 4?
    {
    nWidth4 = Img.cols + (4 - (nWidth4 & 3)); // next int that is div by 4
    // note that we don't bother filling in the final missing columns
    // because FillBorders will overwrite them anyway (TODO correct?)
    pData = (RGBV *)pMalloc(nWidth4 * Img.rows * sizeof(RGBV));
    for(int i = 0; i < Img.rows; i++)
        memcpy(pData    + (i * nWidth4),
               Img.data + (i * Img.cols * sizeof(RGBV)),
               nWidth4 * sizeof(RGBV));
    }
return pData;
}

//-----------------------------------------------------------------------------
static void FillBorders (HDC hdc,
                         int nySrc)
{
HBRUSH hBrush = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
if(fgCenter)
    {
    const MFLOAT xScale = gChildWidth / gSrcWidth;
    if(xgSrc < 0)       // left gap?
        FillRect(hdc, hBrush,
                 0, 0,                               // left, top
                 -xgSrc * xScale, gChildHeight);     // right, bot
    const MFLOAT RightGap = -(gImg.cols - (xgSrc + gSrcWidth));
    if(RightGap > 0)    // right gap?
        FillRect(hdc, hBrush,
                 gChildWidth - RightGap * xScale, 0, // left, top
                 gChildWidth, gChildHeight);         // right, bot
    const MFLOAT yScale = gChildHeight / gSrcHeight;
    const MFLOAT TopGap = -(gImg.rows - (nySrc + gSrcHeight));
    if(TopGap > 0)      // top gap?
        FillRect(hdc, hBrush,
                 0, 0,                               // left, top
                 gChildWidth, TopGap * yScale);      // right, bot
    if(nySrc < 0)       // bottom gap?
       FillRect(hdc, hBrush,
                0, gChildHeight + nySrc * yScale,    // left, top
                gChildWidth, gChildHeight);          // right, bot
    }
else
    {
    if(gDestWidth < gChildWidth)    // right side
        FillRect(hdc, hBrush, gDestWidth, 0, gChildWidth, gChildHeight);
    if(gDestHeight < gChildHeight)  // bottom
        FillRect(hdc, hBrush, 0, gDestHeight, gChildWidth, gChildHeight);
    }
}

//-----------------------------------------------------------------------------
// Convert the mouse position on the screen to the x coordinate in image

static void MouseToImgCoords (
    MFLOAT &x,      // out
    MFLOAT &y,      // out
    MFLOAT xMouse,  // in
    MFLOAT yMouse)  // in
{
x = floor(xgSrc + xMouse * gSrcWidth  / gDestWidth);
y = floor(ygSrc + yMouse * gSrcHeight / gDestHeight);
}

//-----------------------------------------------------------------------------
// This updates the global StretchDIBits parameters:
//    xgSrc, ygSrc, gSrcWidth, gSrcHeight
//    gDestHeight, gDestWidth
//
// These parameters are used both by StretchDIBits and in
// MouseToImgCoords (for converting the mouse coords to image coords).
//
// Note in the first part of this function the code does the following.  If
// the mouse left button is down, then use the old x and y landmark
// positions for positioning the image. This prevents movement of the image
// as you mouse click to change a point's position.  Similarly, in
// JumpToLandmark mode, we keep the screen stable as much as possible But
// we sometimes have to override that, by setting fgForceRecenter.

static void UpdateGlobalCoords (void)
{
static MFLOAT xgLandmarkOld;
xgLandmark = xgLandmarkOld;

static MFLOAT ygLandmarkOld;
ygLandmark = ygLandmarkOld;

if(!(fgLButtonDown || fgJumpToLandmark) || fgForceRecenter)
    {
    // recenter the image on the current landmark (but if igCenterLandmark
    // was specified on the command line, use that instead of the active landmark)

    const int iLandmark = (igCenterLandmark >= 0? igCenterLandmark: igLandmark);
    // Get coords of iLandmark in gShapes[igImg].  xCenter and yCenter
    // are needed for type convert, args to GetPointCoordsFromShape.
    MFLOAT xShapeCoordOfCurrrentLandmark, yShapeCoordOfCurrrentLandmark;
    GetPointCoordsFromShape(
        xShapeCoordOfCurrrentLandmark, yShapeCoordOfCurrrentLandmark,
        gShapes[igImg], iLandmark, true);
    xgLandmark = xShapeCoordOfCurrrentLandmark;
    ygLandmark = yShapeCoordOfCurrrentLandmark;
    xgLandmarkOld = xgLandmark;
    ygLandmarkOld = ygLandmark;
    fgForceRecenter = false;
    }

const MFLOAT ImgHeight = MFLOAT(gImg.rows); // convert to MFLOAT
const MFLOAT ImgWidth = MFLOAT(gImg.cols);

gDestHeight = gChildHeight; gDestWidth  = gChildWidth;
xgSrc = 0; ygSrc = 0; gSrcWidth = gZoomedWidth; gSrcHeight = ImgHeight;

const MFLOAT DestRatio = gChildHeight / gChildWidth;
const MFLOAT ImgRatio  = ImgHeight    / ImgWidth;

if(fgCenter) // put the active landmark in the center of child window
    {
    // Set up StretchDibits so active landmark is in the center of the
    // child window, maintaining the aspect ratio of the original image.
    // Actually we want gSrcHeight divisible by 2 --- therefore sometimes the
    // aspect ratio is slightly off (this prevents y coord fencepost errors).
    gSrcHeight = MFLOAT(((int)(gSrcWidth * DestRatio) / 2) * 2);
    xgSrc = xgLandmark - gSrcWidth/2;
    ygSrc = ygLandmark - gSrcHeight/2;
    }
else  // put the image at the top left of child window, with no zoom
    {
    // We don't center the image so it is clear to the user
    // that we are no longer centering the image.
    if(ImgRatio > DestRatio)                 // image is taller than window?
        gDestWidth = gDestHeight / ImgRatio; // leave right gap
    else                                     // image is narrower than window
        gDestHeight = gDestWidth * ImgRatio; // leave bottom gap
    }
xgSrc       = floor(xgSrc);
ygSrc       = floor(ygSrc);
gSrcWidth   = floor(gSrcWidth);
gSrcHeight  = floor(gSrcHeight);
gDestWidth  = floor(gDestWidth);
gDestHeight = floor(gDestHeight);
}

//-----------------------------------------------------------------------------
static void ChildWm_Paint (HWND hWnd)
{
// if(fgTrace)
//     lprintf("%d ChildWm_Paint\n", ++igDebug);

int nWidth4; // width of image after ensuring width is divisible by 4
RGBV *pData = pGetImg4(nWidth4, gImg); // ensure width is divisible by 4

UpdateGlobalCoords();

BITMAPINFO BmInfo; memset(&BmInfo.bmiHeader, 0, sizeof(BmInfo.bmiHeader));
BmInfo.bmiHeader.biSize     = 40;
BmInfo.bmiHeader.biWidth    = nWidth4;
// note the negative sign here, necessary because
// OpenCV images are upside down wrt windows bitmaps
BmInfo.bmiHeader.biHeight   = -gImg.rows;
BmInfo.bmiHeader.biPlanes   = 1;
BmInfo.bmiHeader.biBitCount = 24;

// following is needed because of negative biHeight (see assign above)
int nySrc = fgCenter?
        int(ygSrc + gImg.rows - 2 * ygLandmark):
        int(ygSrc);

PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);

SetStretchBltMode(hdc, COLORONCOLOR);

// the int casts below prevent compiler warnings but the values are already integral
StretchDIBits(hdc,
    0,               0,                // nxDestUpperLeft, nyDestUpperLeft
    int(gDestWidth), int(gDestHeight), // nDestWidth, nDestHeight
    int(xgSrc),      nySrc,            // nxSrcUpperLeft, nySrcUpperLeft
    int(gSrcWidth),  int(gSrcHeight),  // nSrcWidth, nSrcHeight
    pData,                             // lpBits
    (LPBITMAPINFO)&BmInfo,             // lpBitsInfo
    DIB_RGB_COLORS,                    // wUsage
    SRCCOPY);                          // raser operation code

if(pData != (RGBV *)gImg.data)         // pGetImg4 allocated a new buffer?
    free(pData);

FillBorders(hdc, nySrc); // fill in the edges around image, if any

if(fgTrace) // tiny circle in the center of the window?
    {
    int w2 = int(gChildWidth/2), h2 = int(gChildHeight/2);
    Ellipse(hdc, w2 - 2, h2 - 2, w2 + 2, h2 + 2);
    }
EndPaint(hWnd, &ps);

UpdateDlgWnd(); // will only see the results after ChildWm_Paint returns

if(fgIssueEscMsg)
    {
    if(fgGoBackOnEsc)
        TimedMsgBox("Current image");
    else
        TimedMsgBox("Previous image");
    fgIssueEscMsg = false;
    }
if(fgCheckMode)
    CheckMode_NextImg();
else // while the user is looking at the new image, preload the next.
    PreloadImg();
}

//-----------------------------------------------------------------------------
static void Redisplay (void)
{
DisplayButtons();
LoadCurrentImg();
DisplayTitle(false);

// Mark window contents as needing renewal.   Note that bErase is
// false, because we do all out own edge filling (in FillBorders).

InvalidateRect(hgChildWnd, NULL, false);

// Because the user may be holding the key down (auto-repeat, e.g. holding
// PageDn key to scroll through images) we need to force a screen update --
// else WM_PAINT gets queued behind WM_KEYDOWN and repaint only gets done
// after all keypresses stop.  Hence this call to Wm_Paint.

ChildWm_Paint(hgChildWnd);
}

//-----------------------------------------------------------------------------
static void IssueMarkMsg (void)
{
tprintf(
  "Mark %-10.10s Landmark %d  Position %7.1f %7.1f  (original %7.1f %7.1f)\n",
  sGetCurrentBase(), igLandmark,
  gShapes[igImg](igLandmark, IX),    gShapes[igImg](igLandmark, IY),
  gShapesOrg[igImg](igLandmark, IX), gShapesOrg[igImg](igLandmark, IY));
}


//-----------------------------------------------------------------------------
// fIssueMsgNow=false reduces the number of prints when the mouse is dragged

static void SetLandmarkPosition (
    int    iImg,
    int    iLandmark,
    MFLOAT x,
    MFLOAT y,
    bool   fIssueMsgNow)
{
CV_Assert(iImg == igImg);
CV_Assert(iLandmark == igLandmark);
// point's position changed?
if(fPointUsed(x, y) !=
        fPointUsed(gShapes[igImg](igLandmark, IX),
                   gShapes[igImg](igLandmark, IY)) ||
    cvRound(gShapes[igImg](igLandmark, IX)) != cvRound(x) ||
    cvRound(gShapes[igImg](igLandmark, IY)) != cvRound(y))
    {
    // Position changed: force x and y in range (when zoomed, the user
    // may have clicked off the image).
    bool f = fPointUsed(x, y);
    ForcePointsIntoImg(x, y, gImg.cols, gImg.rows);
    if(f && !fPointUsed(x,y))
        x = XJITTER; // jitter, else will be seen as unused
    gShapes[igImg](igLandmark, IX) = MFLOAT(x);
    gShapes[igImg](igLandmark, IY) = MFLOAT(y);
    fgChangesSaved = false;
    ngLandmarksUpdated++;
    if(fIssueMsgNow)
        IssueMarkMsg();
    else
        fgPendingMarkMsg = true; // want IssueMarkMsg() on mouse up
    // following logic sets fgMoreThanOneLandmarkNbrChanged if necessary
    static int iChangedLandmark = -1;
    if(iChangedLandmark == -1)  // first landmark to be changed?
        iChangedLandmark = iLandmark;
    else if(iLandmark != iChangedLandmark)
        fgMoreThanOneLandmarkNbrChanged = true;
    }
if(fPointUsed(x, y) !=
        fPointUsed(gShapes[igImg](igLandmark, IX),
                   gShapes[igImg](igLandmark, IY)) ||
   cvRound(gShapes[iImg](iLandmark, IX)) !=
                    cvRound(gShapesOrg[iImg](iLandmark, IX)) ||
   cvRound(gShapes[iImg](iLandmark, IY)) !=
                    cvRound(gShapesOrg[iImg](iLandmark, IY)))
    {
    gMarked[iImg](iLandmark, IX) = MARKED; // landmark changed, mark it as such
    }
else
    gMarked[iImg](iLandmark, IX) = gShapesOrg[iImg](iLandmark, IX); // unchanged
}

//-----------------------------------------------------------------------------
static void Wm_Create (HWND hWnd)
{
if(NULL == (hgDlgWnd = CreateDialog(hgInstance, "Dlg", hWnd, DlgProc)))
    Err1("CreateDialog failed");

// the double typecast is necessary to not get a warning in 64 bit builds
HINSTANCE hInstance = HINSTANCE(LONG_PTR(GetWindowLongPtr(hWnd, GWLP_HINSTANCE)));

hgChildWnd = CreateWindow(sgChildWnd,
    NULL,                           // window caption
    WS_CHILDWINDOW | WS_VISIBLE,    // window style
    0,                              // x position
    0,                              // y position
    1,                              // x size
    1,                              // y size
    hWnd,                           // parent window handle
    NULL,                           // window menu handle
    hInstance,                      // program instance handle
    NULL);                          // creation parameters

if(NULL == hgChildWnd)
    Err1("CreateWindow failed for child window");

SetChildCursor();

if(!fgViewMode && !fgSpecialPoints && !fgCheckMode &&
   fFileReadable(sgNewShapeFile) && !fFileWriteable(sgNewShapeFile))
    MsgBox("Note: The shape file\n\n      %s\n\n"
           "is not writeable\n", sgNewShapeFile);
}

//-----------------------------------------------------------------------------
static void TagCurrentShape (bool fTag)
{
ATTR uAttr = uGetAttrFromTag(gTags[igImg]);
ATTR uOldAttr = uAttr;
if(fTag) uAttr |= ugTagAttr; else uAttr &= ~ugTagAttr;
char sNewTag[SLEN]; sprintf(sNewTag, "%8.8x %s", uAttr, sGetCurrentBase());
gTags[igImg] = sNewTag;
if(uAttr != uOldAttr)
    {
    tprintf("New tag %s\n", sNewTag);
    fgChangesSaved = false;
    }
}

//-----------------------------------------------------------------------------
// Return the index of the landmark that is closest to the mouse cursor.
// If any landmarks are Selectable then consider only Selectable landmarks (-H flag).

static int iGetClosestLandmark (void)
{
MFLOAT x1, y1; MouseToImgCoords(x1, y1, xgMouse, ygMouse);

// Adjust coords if mouse if out of the window.  This is a hack to allow
// changing the active landmark when the mouse is out of the window.
// Without this, the out-of-window mouse coords may never get close
// enough to the landmark you are trying to make active.

MFLOAT x = x1, y = y1;
if(xgMouse < 1)
    x = 0;
else if(xgMouse > gChildWidth - 2)
    x = MFLOAT(gImg.cols) - 1;
if(ygMouse < 1)
    y = 0;
else if(ygMouse > gChildHeight - 2)
    y = MFLOAT(gImg.rows) - 1;

MFLOAT Min = FLT_MAX;
int iClosestLandmark = -1;
for(int iLandmark = 0; iLandmark < gShapes[igImg].rows; iLandmark++)
    if(!fgBigH || fgBigHLandmarks[iLandmark])
        {
        MFLOAT x1, y1;
        GetPointCoordsFromShape(x1, y1, gShapes[igImg], iLandmark, true);
        MFLOAT Dist = PointToPointDist(MFLOAT(x), MFLOAT(y), MFLOAT(x1), MFLOAT(y1));
        if(Dist < Min)
            {
            Min = Dist;
            iClosestLandmark = iLandmark;
            }
        }
return iClosestLandmark;
}

//-----------------------------------------------------------------------------
static void HandleJumpToLandmarkMouseMove (void)
{
if(ygMouse < 1)
    ReleaseCapture(); // otherwise the buttons don't work
else
    SetCapture(hgChildWnd);

const int iClosestLandmark = iGetClosestLandmark();
bool fRedisplay = false;
if(iClosestLandmark != igLastClosestLandmark)
    {
    igLastClosestLandmark = iClosestLandmark;
    SetLandmarkNbr(iClosestLandmark);
    fRedisplay = true;
    }
// If mouse is out the window, center the active point (we've held
// of doing that until now to minimize screen movement).
if(xgMouse < 1 || xgMouse > gChildWidth-2 ||
   ygMouse < 1 || ygMouse > gChildHeight-2)
    {
    fRedisplay = true;
    fgForceRecenter = true;
    }
if(fRedisplay)
    Redisplay();
}

//-----------------------------------------------------------------------------
static MFLOAT Signed (unsigned u)
{
if(u > 0x7fff)
    u -= 0x10000;
return MFLOAT(u);
}

//-----------------------------------------------------------------------------
static void HandleMouseMarkLandmark (LPARAM lParam) // left button down
{
xgMouse = Signed(LOWORD(lParam));
ygMouse = Signed(HIWORD(lParam));
if(xgMouse >= 0 && xgMouse < gChildWidth &&
   ygMouse >= 0 && ygMouse < gChildHeight)
    {
    // mouse is in the window
    UpdateGlobalCoords();
    MFLOAT x, y; MouseToImgCoords(x, y, xgMouse, ygMouse);
    if(x == 0 && y == 0)
        x = XJITTER; // jitter, else will be seen as unused
    // capture the mouse even when it is off the window, Petzold Win95 p322
    SetCapture(hgChildWnd);
    SetLandmarkPosition(igImg, igLandmark, x, y, false);
    gMarked[igImg](igLandmark, IX) = MARKED;
    }
}

//-----------------------------------------------------------------------------
static void ChildWm_LButtonDown (LPARAM lParam, bool fSaveForUndo)
{
ClosedTimedMsgBox();
fgPrepareRawImg = true;
if(fgIgnoreClick) // this click is activating the marki window? if so, ignore
    fgIgnoreClick = false;  // next click will not be ignored
else if(fgCheckMode)
    ;
else if(fgViewMode)
    QuickMsgBox("Landmarks locked in \"View\" mode");
else if(fgSpecialPoints)
    QuickMsgBox("Landmarks locked in \"Special Points\" mode");
else if(fgLock)
    TimedMsgBox("Landmarks locked\n\nClick the Unlock button to unlock them");
else if(fFaceDetMat())
    QuickMsgBox("Cannot change a face detector shape");
else
    {
    fgLButtonDown = true;
    if(fSaveForUndo)
        SaveForUndo();
    if(ugTagAttr)
        TagCurrentShape(false);
    else
        HandleMouseMarkLandmark(lParam);
    Redisplay();
    }
}

//-----------------------------------------------------------------------------
static void ChildWm_LButtonUp (LPARAM lParam)
{
if(!fgJumpToLandmark)
    ReleaseCapture();
if(fgCheckMode || fgViewMode || fgSpecialPoints)
    return; // NOTE: return
if(fgPendingMarkMsg)
    {
    fgPendingMarkMsg = false;
    IssueMarkMsg();
    }
fgLButtonDown = false;
if(fgAutoNextImg)
    NextImg(igAutoNextMissingLandmark);
else if(fgAutoNextPoint)
    SetLandmarkNbr(igLandmark+1);
if(fgAutoNextImg || fgAutoNextPoint)
    Redisplay();
}

//-----------------------------------------------------------------------------
static void ChildWm_RButtonDown (void)
{
fgPrepareRawImg = true;
if(fgCheckMode)
   ;
else if(ugTagAttr)
    {
    if(fgViewMode)
        QuickMsgBox("Landmarks locked in \"View\" mode");
    else if(fgSpecialPoints)
        QuickMsgBox("Landmarks locked in \"Special Points\" mode");
    else if(fgLock)
        TimedMsgBox("Landmarks locked --- click the Unlock button to unlock them");
    else if(fFaceDetMat())
        QuickMsgBox("Cannot change a face detector shape");
    else
        TagCurrentShape(true);
    }
else
    fgDark ^= 1;
fgJumpToLandmark = false;
ReleaseCapture();
igLastClosestLandmark = -1;
Redisplay();
}

//-----------------------------------------------------------------------------
static void ChildWm_RButtonUp (void)
{
if(!fgCheckMode && !fgViewMode &&
        !fgSpecialPoints && ugTagAttr && fgAutoNextImg)
    {
    NextImg();
    Redisplay();
    }
}

//-----------------------------------------------------------------------------
static void ChildWm_MouseMove (LPARAM lParam)
{
// Windows feeds us repeated WM_MOUSE messages even when
// the mouse hasn't moved.  Hence the use of lParamOld.

static LPARAM lParamOld;
if(!fgCheckMode && lParam != lParamOld)
    {
    xgMouse = Signed(LOWORD(lParam));
    ygMouse = Signed(HIWORD(lParam));
    lParamOld = lParam;
    // !fgLButtonDown prevents landmarking switching while dragging
    if(fgJumpToLandmark && !fgLButtonDown)
        HandleJumpToLandmarkMouseMove();
    // if(fgTrace) // show mouse coords in window title?
    if(1) // TODO always show mouse coords
        DisplayTitle(true);
    if(fgLButtonDown)
        ChildWm_LButtonDown(lParam, false);
    }
}

//-----------------------------------------------------------------------------
static LRESULT CALLBACK  // for hgChildWnd
ChildWndProc (HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
static bool fOldJumpToLandmark;
switch(nMsg)
    {
    case WM_KEYDOWN:        // user hit a key
    case WM_KEYUP:          // user released key
    case WM_CHAR:
    case WM_SYSKEYDOWN:
        // send most messages to parent (hgMainWnd)
        SendMessage(GetParent(hWnd), nMsg, wParam, lParam);
        return 0;
    case WM_PAINT:
        ChildWm_Paint(hWnd);    // this updates the picture the user sees
        return 0;
    case WM_LBUTTONDOWN:        // mouse left click
        ChildWm_LButtonDown(lParam, true);
        break;
    case WM_LBUTTONUP:          // mouse left click up
        ChildWm_LButtonUp(lParam);
        break;
    case WM_RBUTTONDOWN:        // mouse right click
        ChildWm_RButtonDown();
        break;
    case WM_RBUTTONUP:          // mouse right click up
        ChildWm_RButtonUp();
        break;
    case WM_MBUTTONDOWN:        // mouse middle click
        Redisplay();            // this is needed to fix cursor, not sure why
        break;
    case WM_MOUSEMOVE:
        ChildWm_MouseMove(lParam);
        break;
    case WM_MOUSEWHEEL:
        fgPrepareRawImg = false;
        if(short(HIWORD(wParam)) > 0)
            SendMessage(hWnd, WM_KEYDOWN, VK_NEXT, 0);  // PageDn
        else
            SendMessage(hWnd, WM_KEYDOWN, VK_PRIOR, 0); // PageUp
        return 0;
    case WM_KILLFOCUS:
        fOldJumpToLandmark = fgJumpToLandmark;
        fgJumpToLandmark = false;
        ReleaseCapture();
        DisplayButtons();
        return 0;
    case WM_SETFOCUS:
        fgJumpToLandmark = fOldJumpToLandmark;
        if(fgJumpToLandmark)
            fgShowActiveNbr = true;
        DisplayButtons();
        return 0;
    }
return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

//-----------------------------------------------------------------------------
static LRESULT CALLBACK // for hgMainWnd
WndProc (HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
switch(nMsg)
    {
    case WM_CREATE:
        Wm_Create(hWnd);
        return 0;
    case WM_SIZE:
        Wm_Size(hWnd, lParam);
        return 0;
    case WM_MOVE: // sent after the window has been moved
        Wm_Move(hWnd, lParam);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        {
        if(!fgCheckMode) // already isssued the time message in check mode
            IssueElapsedTimeMsg();
        else if(igImg != 0 && NSIZE(gTags))
            tprintf("Checking stopped at %s\n", sGetCurrentBase());
        if(!fCheckSaveState(true))
            return 0;   // cancel
        // ensure dlg wind not minimized before saving its pos to registry
        ShowWindow(hgDlgWnd, SW_RESTORE);
        SaveStateToRegistry();
        break;          // call default window close handler
        }
    case WM_SETFOCUS:   // set focus to child window
        SetFocus(hgChildWnd);
        return 0;
    case WM_COMMAND:    // toolbar buttons
        Wm_Command(hWnd, nMsg, wParam, lParam);
        return 0;
    case WM_NOTIFY:     // display a tooltip
        Wm_NotifyMarki(hWnd, nMsg, wParam, lParam, sgTooltips, IDM_FirstInToolbar);
        return 0;
    case WM_KEYDOWN:        // user hit a key
        Wm_Keydown(hWnd, wParam);
        return 0;
    case WM_KEYUP:          // user released key
        if(wParam == VK_SHIFT)
            fgShiftKeyDown = false;
        else if(wParam == VK_CONTROL)
            fgCtrlKeyDown = false;
        return 0;
    case WM_CHAR:
        Wm_Char(hWnd, wParam);
        return 0;
    case WM_ACTIVATE:
        if(wParam == WA_CLICKACTIVE)
            fgIgnoreClick = true;
        break;
    }
return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Create the same window layout as last time by looking at the registry.
// Also get toolbar button settings.
// Init to defaults if nothing in registry or global flag fgUseRegEntries==false

static void GetStateFromRegistry (
    int *pxPos,     // all out
    int *pyPos,
    int *pxSize,
    int *pySize,
    int *pxDlg,
    int *pyDlg)
{
HKEY  hKey = NULL;
DWORD nBuf = SLEN;
BYTE  Buf[SLEN];
char sVersion[SLEN];  sVersion[0] = 0; // e.g. "310j"
char sShapeFile[SLEN]; sShapeFile[0] = 0;
int xPos, yPos, xSize = -1, ySize = -1;
int xDlg = -1, yDlg = -1;
int iImg = -1, iLandmark = igLandmark;
// we use int instead of bool for these because you can't scanf a bool
int fGray, fDark, fShowActiveNbr, fShowLandmarks, fShowNbrs, fConnectDots, fCenter;
int fAutoNextImg, fAutoNextPoint;
// likewise, you can't scanf a double
float Zoom = -1;

const bool fInitialUseRegEntries = fgUseRegEntries;

RECT RectWorkArea;
if(0 == SystemParametersInfo(SPI_GETWORKAREA, 0, &RectWorkArea, 0))
    lprintf("\nSystemParametersInfo SPI_GETWORKAREA failed\n");

logprintf("Getting options from registry HKEY_CURRENT_USER %s\\%s\n",
          sgRegistryKey, sgRegistryName);

if(fgUseRegEntries &&
        ERROR_SUCCESS !=
            RegOpenKeyEx(HKEY_CURRENT_USER, sgRegistryKey, 0, KEY_READ, &hKey))
    {
    lprintf("\nRegOpenKeyEx failed "
            "(HKEY_CURRENT_USER Key \"%s\" Name \"%s\")\n",
            sgRegistryKey, sgRegistryName);
    fgUseRegEntries = false;
    }
if(fgUseRegEntries &&
        ERROR_SUCCESS !=
            RegQueryValueEx(hKey, sgRegistryName, NULL, NULL, Buf, &nBuf))
    {
    lprintf("\nRegQuertyValueEx failed "
            "(HKEY_CURRENT_USER Key \"%s\" Name \"%s\")\n",
            sgRegistryKey, sgRegistryName);
    fgUseRegEntries = false;
    }
if(fgUseRegEntries)
    RegCloseKey(hKey);

if(fgUseRegEntries &&
        20 != sscanf((char *)Buf,
                    "%s %s "
                    "%d %d %d %d "
                    "%d %d "
                    "%d %d "
                    "%d %d %d %d "
                    "%d %d %d "
                    "%d %d "
                    "%f ",
            sVersion, sShapeFile,
            &xPos, &yPos, &xSize, &ySize,
            &xDlg, &yDlg,
            &iImg, &iLandmark,
            &fGray, &fDark, &fShowActiveNbr, &fShowLandmarks,
            &fShowNbrs, &fConnectDots, &fCenter,
            &fAutoNextImg, &fAutoNextPoint,
            &Zoom))
    {
    lprintf("\nRegistry entry invalid "
            "(HKEY_CURRENT_USER Key \"%s\" Name \"%s\")\n",
            sgRegistryKey, sgRegistryName);
    fgUseRegEntries = false;
    }
if(fgUseRegEntries &&
           (sShapeFile[0] == 0               ||
            xPos + xSize < 20                ||
            yPos + ySize < 20                ||
            xSize < 20                       ||
            ySize < 20                       ||
            iImg < 0 || iImg > 1e6           ||
            iLandmark < 0 || iLandmark > 1e3 ||
            !fBool(fGray)                    ||
            !fBool(fDark)                    ||
            !fBool(fShowActiveNbr)           ||
            !fBool(fShowLandmarks)           ||
            !fBool(fShowNbrs)                ||
            !fBool(fConnectDots)             ||
            !fBool(fCenter)                  ||
            !fBool(fAutoNextImg)             ||
            !fBool(fAutoNextPoint)           ||
            Zoom < 0 || Zoom > MAX_ZOOM))
    {
    lprintf("\nRegistry entry out-of-range "
            "(HKEY_CURRENT_USER Key \"%s\" Name \"%s\")\n",
            sgRegistryKey, sgRegistryName);
    fgUseRegEntries = false;
    }
if(fgUseRegEntries &&
    (xSize > RectWorkArea.right - RectWorkArea.left + 100 ||
     ySize > RectWorkArea.bottom - RectWorkArea.top + 100 ||
     xPos < RectWorkArea.left - xSize + 40                ||
     yPos < RectWorkArea.top - ySize + 40                 ||
     xDlg < RectWorkArea.left - ngDlgWidth + 40           ||
     yDlg < RectWorkArea.top - ngDlgHeight + 40))
    {
    lprintf("\nRegistry entry window position invalid "
            "(HKEY_CURRENT_USER Key \"%s\" Name \"%s\")\n",
            sgRegistryKey, sgRegistryName);
    fgUseRegEntries = false;
    }
if(!fgUseRegEntries && fInitialUseRegEntries)
    {
    lprintf("No problem, will use the default settings "
            "(probably running %s for the first time)\n\n",
            sgAppName);

    lprintf("Buffer: %s\n", Buf);

    lprintf("sVersion %s sShapeFile %s\n"
            "xPos %d yPos %d xSize %d ySize %d\n"
            "xDlg %d yDlg %d\n"
            "iImg %d iLandmark %d\n"
            "fGray %d fDark %d fShowActiveNbr %d fShowLandmarks %d\n"
            "fShowNbrs %d fConnectDots %d fCenter %d\n"
            "fAutoNextImg %d fAutoNextPoint %d\n"
            "Zoom %g\n\n",
        sVersion, sShapeFile,
        xPos, yPos, xSize, ySize,
        xDlg, yDlg,
        iImg, iLandmark,
        fGray, fDark, fShowActiveNbr, fShowLandmarks,
        fShowNbrs, fConnectDots, fCenter,
        fAutoNextImg, fAutoNextPoint,
        Zoom);
    }
if(!fgUseRegEntries)
    {
    xSize = CLAMP(RectWorkArea.right  - RectWorkArea.left, 500, 800);
    ySize = CLAMP(RectWorkArea.bottom - RectWorkArea.top,  500, 1080);
    xPos = RectWorkArea.right - xSize;
    yPos = RectWorkArea.top;
    xDlg = RectWorkArea.right - ngDlgWidth - 6;
    yDlg = ySize - ngDlgHeight - 6;
    if(igLandmark < 0) // not initialized by -l flag?
        iLandmark = 0;
    iImg = igImg;
    fGray = fgGray;
    fDark = fgDark;
    fShowActiveNbr = fgShowActiveNbr;
    fShowLandmarks = fgShowAllDots;
    fShowNbrs = fgShowNbrs;
    fConnectDots = fgConnectDots;
    fCenter = fgCenter;
    fAutoNextImg = fgAutoNextImg;
    fAutoNextPoint = fgAutoNextPoint;
    Zoom = (float)gZoomAmount;
    }
*pxPos  = xPos;
*pyPos  = yPos;
*pxSize = xSize;
*pySize = ySize;
*pxDlg  = xDlg;
*pyDlg  = yDlg;
igImg = iImg;
igLandmark = iLandmark;
fgGray          = fGray != 0;
fgDark          = fDark != 0;
fgShowActiveNbr = fShowActiveNbr != 0;
fgShowAllDots   = fShowLandmarks != 0;
fgShowNbrs      = fShowNbrs != 0;
fgConnectDots   = fConnectDots != 0;
fgCenter        = fCenter != 0;
fgAutoNextImg   = fAutoNextImg != 0;
fgAutoNextPoint = fAutoNextPoint != 0;
gZoomAmount     = Zoom;
}

//-----------------------------------------------------------------------------
static void SaveStateToRegistry (void)
{
logprintf("Saving options to registry HKEY_CURRENT_USER %s\\%s\n",
          sgRegistryKey, sgRegistryName);

RECT rectMain; GetWindowRect(hgMainWnd, &rectMain);
RECT rectDlg;  GetWindowRect(hgDlgWnd, &rectDlg);
char sVersion[SLEN]; sprintf(sVersion, "%s%c", STASM_VERSION, sgVersionAppend[0]);

char sRegValue[SLEN];
sprintf(sRegValue,
        "%s %s "
        "%d %d %d %d "
        "%d %d "
        "%d %d "
        "%d %d %d %d "
        "%d %d %d "
        "%d %d "
        "%g ",
    sVersion, sgShapeFile,
    rectMain.left, rectMain.top,
    rectMain.right-rectMain.left, rectMain.bottom-rectMain.top,
    rectDlg.left, rectDlg.top,
    igImg, igLandmark,
    fgGray, fgDark, fgShowActiveNbr, fgShowAllDots,
    fgShowNbrs, fgConnectDots, fgCenter,
    fgAutoNextImg, fgAutoNextPoint,
    gZoomAmount);

// no point in checking return values in func calls below
// because if they fail there is not much we can do

HKEY hKey;
DWORD dwDispo;
RegCreateKeyEx(HKEY_CURRENT_USER, sgRegistryKey, 0, "", 0,
    KEY_ALL_ACCESS, NULL, &hKey, &dwDispo);
RegSetValueEx(hKey, sgRegistryName, 0, REG_SZ, (CONST BYTE *)sRegValue,
    STRLEN(sRegValue)+1);
RegCloseKey(hKey);
}

//-----------------------------------------------------------------------------
static void StripQuotesAndBackQuote (char sStripped[], const char *sQuoted)
{
// remove quotes at each end of string, if any

strcpy(sStripped, &sQuoted[sQuoted[0] == '"'? 1:0]); // discard initial "
int iLen = STRLEN(sStripped)-1;
if(iLen > 0 && sStripped[iLen] == '"')
    sStripped[iLen] = 0;                             // discard final "

// replace backquote with space

for(int i = 0; sStripped[i]; i++)
    if(sStripped[i] == '`')
        sStripped[i] = ' ';
}

//-----------------------------------------------------------------------------
// Initialize fgSpecialPoints (true if any points have a SPECIAL_X_OFFSET)
// and gSpecialPoints (boolean which says which points that had the offset).
// This also removes the offset from the points in gShapesOrg.

static void PossiblyInitSpecialPoints (void)
{
DimClear(gSpecialPoints, gShapesOrg[0].rows, 1);
fgSpecialPoints = false;
for(int iLandmark = 0; iLandmark < gShapesOrg[0].rows; iLandmark++)
    if(gShapesOrg[0](iLandmark, IX) > SPECIAL_X_OFFSET - 10000)
        {
        gSpecialPoints(iLandmark) = 1;
        fgSpecialPoints = true;
        }

if(fgSpecialPoints)
    {
    // remove offset from all special points
    for(int iShape = 0; iShape < ngImgs; iShape++)
        for(int iLandmark = 0; iLandmark < gShapesOrg[iShape].rows; iLandmark++)
            if(gShapesOrg[iShape](iLandmark, IX) > SPECIAL_X_OFFSET - 10000)
                gShapesOrg[iShape](iLandmark, IX) -= SPECIAL_X_OFFSET;
    }
}

//-----------------------------------------------------------------------------
static void InitGlobals (void)
{
if(igImg < 0)  // not set by user or registry?
    igImg = 0;
else if(igImg >= ngImgs)
    {
    // can happen if shape file is not same as last time
    tprintf("Image index %d from last invocation of Marki is out of range, "
            "setting to 0\n", igImg);
    SetImgNbr(0);
    }
igFirstImg = igImg;
const int nLandmarks = gShapes[igImg].rows;
if(igLandmark < 0) // not  set by user or registry?
    igLandmark = 0;
if(igLandmark >= nLandmarks)
    {
    tprintf("Landmark %d is out of range, forcing to 0\n", igLandmark);
    igLandmark = 0;
    }
if(igCenterLandmark >= nLandmarks) // already tested less than 0
    {
    tprintf("Landmark %d specified by -c is out of range, ignoring\n",
            igCenterLandmark);
    igCenterLandmark = -1;
    }
#if 0
if(igAutoNextMissingLandmark >= nLandmarks) // already tested less than 0
    {
    tprintf("Landmark %d specified by -m is out of range, ignoring\n",
            igAutoNextMissingLandmark);
    igAutoNextMissingLandmark = -1;
    }
#endif
if(igCmdLineLandmark >= nLandmarks) // already tested less than 0
    {
    tprintf("Landmark %d specified by -l is out of range, forcing to 0\n",
            igCmdLineLandmark);
    igCmdLineLandmark = 0;
    }
if(fgBigH)
    {
    if(igCmdLineLandmark >= 0 && !fgBigHLandmarks[igCmdLineLandmark])
        Err1("Landmark specified by -l is not one of those specified by -H");
    // ensure that the active landmark is one of the "Selectable" landmarks
    if(!fgBigHLandmarks[igLandmark])
        for(igLandmark = 0; igLandmark < nLandmarks; igLandmark++)
            if(fgBigHLandmarks[igLandmark])
                break;
    }
else if(fgSmallH)
    {
    if(igCmdLineLandmark >= 0 && !fgSmallHLandmarks[igCmdLineLandmark])
        Err1("Landmark specified by -l is not one of those specified by -h");
    // ensure that the active landmark is one of the highlighted landmarks
    if(!fgSmallHLandmarks[igLandmark])
        for(igLandmark = 0; igLandmark < nLandmarks; igLandmark++)
            if(fgSmallHLandmarks[igLandmark])
                break;
    }
ugAttr = uGetAttrFromTag(gTags[igImg]); // update ugAttr attribute bits from tag
if(fFaceDetMat())
    fgShowAllDots = fgConnectDots = true;
if(fgViewMode)
    fgLock = true; // will stay locked the entire session
if(fgCheckMode)
    {
    fgLock = true;
    fgAutoNextPoint = false;
    }
else if(fgSpecialPoints)
    {
    // not all the following assignments may actually be necessary
    fgLock = true;
    fgShowAllDots = fgConnectDots = fgShowNbrs = false;
    fgAutoNextPoint = fgAllButtons = false;
    }
if(!fgAllButtons)
    fgAutoNextPoint = false; // no auto-next-landmark
}

//-----------------------------------------------------------------------------
static void CheckPoseSpec (
    VEC        &PoseSpec,
    int        iMin,
    int        iMax,
    const char sPose[])
{
if(PoseSpec(0,iMin) > PoseSpec(0,iMax))
    Err("Min%s %g is greater than Max%s %g",
        sPose, PoseSpec(0,iMin), sPose, PoseSpec(0,iMax));
if(PoseSpec(0,iMin) < -90 || PoseSpec(0,iMin) > 90)
    Err("Min%s %g is out of range", sPose, PoseSpec(0,iMin));
if(PoseSpec(0,iMax) < -90 || PoseSpec(0,iMax) > 90)
    Err("Max%s %g is out of range", sPose, PoseSpec(0,iMax));
}

//-----------------------------------------------------------------------------
static void ReadCmdLine (
    const LPSTR sCmdLine)
{
char *sWhiteSpace = " \t";
char sStripped[MAX_PRINT_LEN]; // allow long regular expressions for -p flag

// A hack to deal with strtok: convert spaces inside quotes to backquote
// This allows us to place spaces inside strings in quotes (for regular
// expression and filenames)

int Len = STRLEN(sCmdLine);
int i;
for(i = 0; i < Len; i++)
    if(sCmdLine[i] == '"')
        {
        i++;
        while (sCmdLine[i] && sCmdLine[i] != '"')
            {
            if(sCmdLine[i] == ' ')
                sCmdLine[i] = '`';
            i++;
            }
        }
char *sToken = strtok(sCmdLine, sWhiteSpace);
while (sToken != NULL)
    {
    bool fAlreadyGotNextToken = false;
    if(sToken[0] == '-')
        {
        if(sToken[1] == 0)
            Err1(sgUsage);
        switch(sToken[1])
            {
            case 'B':
                if(sToken[2] != 0) // can't concatenate flags
                    Err1(sgUsage);
                fgAllButtons = true;
                break;
            case 'C':
                if(sToken[2] != 0)
                    Err1(sgUsage);
                fgCheckMode = true;
                fgUseRegEntries = false;
                break;
            case 'F':
                if(sToken[2] != 0)
                    Err1(sgUsage);
                fgUseRegEntries = false;
                break;
            // case 'G':
            //     if(sToken[2] != 0)
            //         Err1(sgUsage);
            //     fgShowUnusedPointsInGreen = true;
            //     break;
            case 'V':
                if(sToken[2] != 0)
                    Err1(sgUsage);
                fgViewMode = true;
                fgAllButtons = true;
                break;
            case 'H':
                {
                fgAllButtons = true;
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL)
                    Err1(sgUsage);
                do
                    {
                    int iLandmark = -1;
                    sscanf(sToken, "%d", &iLandmark);
                    if(iLandmark < 0 || iLandmark >= ngMaxNbrLandmarks)
                        Err1("Illegal landmark number for -h flag.  "
                               "Use -? for help.");
                    fgBigHLandmarks[iLandmark] = true;
                    fgBigH = true;
                    if(igLandmark < -1) // not set by user?
                        igLandmark = iLandmark;
                    fAlreadyGotNextToken = true;
                    sToken = strtok(NULL, sWhiteSpace);
                    }
                while(sToken && fDigit(sToken[0]));
                }
                break;
            case 'h':
                {
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL)
                    Err1(sgUsage);
                do
                    {
                    int iLandmark = -1;
                    if(sToken[0] == '*' && sToken[1] == 0)
                        for(i = 0; i < NELEMS(fgSmallHLandmarks); i++)
                            fgSmallHLandmarks[i] = true;
                    else
                        {
                        sscanf(sToken, "%d", &iLandmark);
                        if(iLandmark < 0 || iLandmark >= ngMaxNbrLandmarks)
                            Err1("Illegal landmark number for -h flag.  "
                                   "Use -? for help.");
                        if(igLandmark < -1) // not set by user?
                            igLandmark = iLandmark;
                        }
                    fgSmallHLandmarks[iLandmark] = true;
                    fgSmallH = true;
                    fAlreadyGotNextToken = true;
                    sToken = strtok(NULL, sWhiteSpace);
                    }
                while(sToken && fDigit(sToken[0]));
                }
                break;
            case 'c':
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL)
                    Err1(sgUsage);
                igCenterLandmark = -1;
                sscanf(sToken, "%d", &igCenterLandmark);
                if(igCenterLandmark < 0 || igCenterLandmark >= ngMaxNbrLandmarks)
                    Err1("Illegal landmark number for -c flag.  Use -? for help.");
                break;
            case 'l':
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL)
                    Err1(sgUsage);
                igCmdLineLandmark = -1;
                sscanf(sToken, "%d", &igCmdLineLandmark);
                if(igCmdLineLandmark < 0 || igCmdLineLandmark >= ngMaxNbrLandmarks)
                    Err1("Illegal landmark number for -l flag.  Use -? for help.");
                igLandmark = igCmdLineLandmark;
                break;
#if 0
            case 'm':
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL)
                    Err1(sgUsage);
                igAutoNextMissingLandmark = -1;
                sscanf(sToken, "%d", &igAutoNextMissingLandmark);
                if(igAutoNextMissingLandmark < 0 ||
                        igAutoNextMissingLandmark >= ngMaxNbrLandmarks)
                    Err1("Illegal landmark number for -m flag.  Use -? for help.");
                igLandmark = igAutoNextMissingLandmark;
                break;
#endif
            case 'o':
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL)
                    Err1(sgUsage);
                StripQuotesAndBackQuote(sStripped, sToken);
                if(!sStripped || sStripped[0] == 0 || sStripped[0] == '-')
                    Err1("Illegal filename for -o flag.  Use -? for help.");
                strcpy(sgNewShapeFile, sStripped);
                break;
            case 'p':
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL)
                    Err1(sgUsage);
                StripQuotesAndBackQuote(sStripped, sToken);
                if(!sStripped || sStripped[0] == 0 || sStripped[0] == '-')
                    Err1("Illegal string for -p flag.  Use -? for help.");
                strcpy(sgTagRegex, sStripped);
                break;
            case 'P':
                if(sToken[2] != 0)
                    Err1(sgUsage);
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL || 1 != sscanf(sToken, "%x", &ugMask0))
                    Err1("-P needs two hex numbers.  Use -? for help.");
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL || 1 != sscanf(sToken, "%x", &ugMask1))
                    Err1("-P needs two hex numbers.  Use -? for help.");
                break;
            case 'q':       // -q[!] YawMin YawMax PitchMin PitchMax RollMin RollMax
                if(fgPoseSpecifiedByUser)
                    Err("-q specified twice");
                fgPoseSpecifiedByUser = true;
                if(sToken[2] != 0 && sToken[2] != '!')
                    Err1(sgUsage);
                fgInvertPoseSpec = sToken[2] == '!';
                for(int i = 0; i < 6; i++)
                    {
                    sToken = strtok(NULL, sWhiteSpace);
                    if(sToken == NULL || 1 != sscanf(sToken, "%x", &ugMask0))
                        Err1("-q needs six numbers.  Use -? for help.");
                    float Temp; // you can't scanf a double so use a float
                    if(1 != sscanf(sToken, "%f", &Temp))
                        Err("Illegal pose \"%s\" for -q (-q needs six numbers)\n\n"
                            "Use -? for help.", sToken);
                    gPoseSpec(i) = Temp;
                    }
                CheckPoseSpec(gPoseSpec, 0, 1, "Yaw");
                CheckPoseSpec(gPoseSpec, 2, 3, "Pitch");
                CheckPoseSpec(gPoseSpec, 4, 5, "Roll");
                break;
            case 't':
                sToken = strtok(NULL, sWhiteSpace);
                if(sToken == NULL || 1 != sscanf(sToken, "%x", &ugTagAttr))
                    Err1("-t needs a number.  Use -? for help.");
                if(ugTagAttr == 0)
                    Err1("0 cannot be used as a tag for -t.  Use -? for help.");
                break;
            default:    // bad flag
                Err1(sgUsage);
            }
        }
    else // assume a string without '-' is the shape file name
        {
        StripQuotesAndBackQuote(sgShapeFile, sToken);
        strcpy(sgShapeFile, sToken);
        }
    if(!fAlreadyGotNextToken)
        sToken = strtok(NULL, sWhiteSpace);
    if(sgShapeFile[0] && sToken)
        Err1("Flags not allowed after the shape file, or "
               "extra tokens on the command line");
    }
}

//-----------------------------------------------------------------------------
static void Init (
    const LPSTR sCmdLine)
{
ReadCmdLine(sCmdLine);

// Shutdown if this program is running already (else the log files
// get mixed up). But in view mode we don't create a log file so ok
// to run twice.  Note that we can only do this after processing
// the command line flags because we check fgViewMode.
if(!fgViewMode)
   {
   CreateMutex(NULL, true, sgAppName);
   if(GetLastError() == ERROR_ALREADY_EXISTS)
       Err1("Marki is running already\n(Use -V flag to run Marki twice)");
   }
if(!sgShapeFile[0])    // no shape file specified
    Err1("You must specify a shape file.  Use marki -? for help.\n"
        "\n"
        "Example 1: marki myshapes.shape  (will save results in the same file)\n"
        "\n"
        "Example 2: marki -o temp.shape -p \" i[^r]\" ../data/muct68.shape");

if(fgViewMode && fgCheckMode) // don't want check mode without log file
    Err1("You cannot use both -V and -C");

if(sgNewShapeFile[0] == 0) // -o flag not used?
    {
    if(sgTagRegex[0] || ugMask0 != 0 || ugMask1 != 0 || fgPoseSpecifiedByUser)
        {
        // not all shapes were loaded, create a new shape file to prevent
        // overwriting the original shape file with an incomplete set of shapes
        char sDrive1[_MAX_DRIVE], sDir1[_MAX_DIR], sBase1[_MAX_FNAME], sExt1[_MAX_EXT];
        splitpath(sgShapeFile, sDrive1, sDir1, sBase1, sExt1);
        makepath(sgNewShapeFile, sDrive1, sDir1, ssprintf("short_%s", sBase1), sExt1);
        if(!fgViewMode) // in view mode the output file is not actually used, so no comment here
            lprintf("\n\nNote: The output file name intentionally differs from "
                    "the input file name\n\n");
        }
    else
        strcpy(sgNewShapeFile, sgShapeFile);
    }
lprintf("Output shape file %s%s\n", sgNewShapeFile,
        fgViewMode || fgCheckMode? " (will not be used because -V or -C flag was used)": "");
if(ugMask0 || ugMask1)
    {
    lprintf("Matching ");
    if(ugMask0)
        lprintf("Mask0 %x [%s] ", ugMask0, sAttrAsString(ugMask0));
    if(ugMask1)
        lprintf("Mask1 %x [%s]", ugMask1, sAttrAsString(ugMask1, true));
    lprintf("\n");
    }
if(sgTagRegex[0])
    lprintf("Regex \"%s\"\n", sgTagRegex);
if(ugTagAttr)
    lprintf("TagMode with tag bit %x [%s]\n", ugTagAttr, sAttrAsString(ugTagAttr));

if(fgViewMode) // in view mode don't overwrite any other possible incarnation of Marki
    tprintf("View mode (will not create %s)\n", sgMarkiLog);
else // note: pgLogFile is defined in util.cpp
    if(NULL == (pgLogFile = fopen(sgMarkiLog, "wt")))
        Err1("Cannot open log file %s", sgMarkiLog);

gStartTime = clock();

if(pgLogFile)
    if(fprintf(pgLogFile, "%s version %s%s\n",
               sgAppName, STASM_VERSION, sgVersionAppend) < 0)
        Err1("Cannot write to the log file %s", sgMarkiLog);

WNDCLASSEX wndclass;
wndclass.lpszClassName = sgAppName;
wndclass.cbSize        = sizeof(wndclass);
wndclass.style         = CS_HREDRAW | CS_VREDRAW;
wndclass.lpfnWndProc   = WndProc;
wndclass.cbClsExtra    = 0;
wndclass.cbWndExtra    = 0;
wndclass.hInstance     = hgInstance;
wndclass.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
wndclass.lpszMenuName  = NULL;
wndclass.hCursor       = LoadCursor(NULL, IDC_CROSS);
wndclass.hIcon         = LoadIcon(hgInstance, sgAppName);
wndclass.hIconSm       = LoadIcon(hgInstance, sgAppName);

if(!RegisterClassEx(&wndclass))
    Err1("RegisterClass failed");

// Create the class for the child (image) window, we will create
// the actual window later in when the main window gets a WM_CREATE

wndclass.lpfnWndProc   = ChildWndProc;
wndclass.hIcon         = NULL;
wndclass.hIconSm       = NULL;
wndclass.hbrBackground = NULL; // we do all our own painting
wndclass.lpszClassName = sgChildWnd;

if(!RegisterClassEx(&wndclass))
    Err1("RegisterClass failed for the child window");

// Use the same window layout as last time by looking at registry.
// This also sets a whole bunch of globals like igImg and igLandmark.

int xPos, yPos, xSize, ySize;
GetStateFromRegistry(&xPos, &yPos, &xSize, &ySize, &xgDlg, &ygDlg);

hgMainWnd = CreateWindow(sgAppName,
        sgAppName,          // window caption
        WS_OVERLAPPEDWINDOW,    // window style
        xPos,                   // x position
        yPos,                   // y position
        xSize,                  // x size
        ySize,                  // y size
        NULL,                   // parent window handle
        NULL,                   // window menu handle
        hgInstance,             // program instance handle
        NULL);                  // creation parameters

if(!hgMainWnd)
    Err1("CreateWindowEx failed");
hgToolbar = hCreateToolbar(hgToolbarBmp,
                           hgMainWnd,
                           fgAllButtons? gToolbarButtonsAll: gToolbarButtons,
                           IDR_TOOLBAR_BITMAP);
CreateMyCursor(hgInstance);
ReadSelectedShapes(gShapesOrg, gTags, sgImgDirs,
                   sgShapeFile, 0, sgTagRegex, ugMask0, ugMask1,
                   fgPoseSpecifiedByUser? &gPoseSpec: NULL, fgInvertPoseSpec);
ngImgs = NSIZE(gShapesOrg);
CV_Assert(ngImgs);
PossiblyInitSpecialPoints();
Copy(gShapes, gShapesOrg);
Copy(gMarked, gShapesOrg);
InitGlobals();
LoadCurrentImg();
if(fgCheckMode)
    {
    if(igFirstImg == 0)
        tprintf("Check started at the first image\n");
    else
        tprintf("Check started at %s\n", sGetCurrentBase());
    }
DisplayTitle(false);
ShowWindow(hgMainWnd, SW_SHOW);
UpdateWindow(hgMainWnd);
DisplayButtons();
}

//-----------------------------------------------------------------------------
static void WinMain1 (const LPSTR sCmdLine)
{
Init(sCmdLine);
MSG msg;
while(GetMessage(&msg, NULL, 0, 0))
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }
}

//-----------------------------------------------------------------------------
// This application calls Stasm's internal routines.  Thus we need to catch a
// potential throw from Stasm's error handlers.  Hence the try/catch code below.

int WINAPI WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     sCmdLine,
    int       iCmdShow)
{
sgAppName = "Marki";
hgInstance = hInstance;
fgGraphicalApp = true; // this is a graphical app, so handle errors accordingly
CatchErrs();
try
    {
    WinMain1(sCmdLine);
    }
catch(...)
    {
    lprintf("\n");
    MsgBox("Error: %s", sStasmCatch()); // will call lprintf too
    exit(-1);   // failure
    }
return 0;       // success
}
