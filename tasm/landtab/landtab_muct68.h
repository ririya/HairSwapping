// landtab_muct68.h: definitions for XM2VTS and MUCT 68 point shapes
//
// Copyright (C) 2005-2013, Stephen Milborrow

#ifndef STASM_LANDTAB_MUCT68_H
#define STASM_LANDTAB_MUCT68_H

// Note that the AT_Hat bit is ignored if pyr lev > HAT_START_LEV

static const LANDMARK_INFO LANDMARK_INFO_TAB[68] = // 68 points
{
//   par pre next weight bits
    { 14,  1, 32,  1., AT_Beard|AT_Glasses      }, // 00 LTemple
    { 13, -1, -1,  1., AT_Beard|AT_Glasses      }, // 01 LJaw1
    { 12, -1, -1,  1., AT_Beard                 }, // 02 LJaw2
    { 11, -1, -1,  1., AT_Beard                 }, // 03 LJaw3
    { 10, -1, -1,  1., AT_Beard                 }, // 04 LJaw4
    { 9,  -1, -1,  1., AT_Beard                 }, // 05 LJaw5
    { 8,  -1, -1,  1., AT_Beard                 }, // 06 LJaw6
    { -1, -1, -1,  1., AT_Beard                 }, // 07 CTipOfChin
    { 6,  -1, -1,  1., AT_Beard                 }, // 08 RJaw6
    { 5,  -1, -1,  1., AT_Beard                 }, // 09 RJaw5
    { 4,  -1, -1,  1., AT_Beard                 }, // 10 RJaw4
    { 3,  -1, -1,  1., AT_Beard                 }, // 11 RJaw3
    { 2,  -1, -1,  1., AT_Beard                 }, // 12 RJaw2
    { 1,  -1, -1,  1., AT_Beard|AT_Glasses      }, // 13 RJaw1
    { 0,  13, 27,  1., AT_Beard|AT_Glasses      }, // 14 RTemple
    { 21, 14, 16,  1., AT_Glasses|AT_Hat        }, // 15 REyebrowOuter
    { 22, 14, 17,  1., AT_Glasses|AT_Hat        }, // 16 REyebrowTopOuter
    { 23, 16, 18,  1., AT_Glasses|AT_Hat        }, // 17 REyebrowTopInner
    { 24, 17, 29,  1., AT_Glasses|AT_Hat        }, // 18 REyebrowInner
    { 25, 18, 20,  1., AT_Glasses|AT_Hat        }, // 19 Point19
    { 26, 27, 32,  1., AT_Glasses|AT_Hat        }, // 20 Point20
    { 15,  0, 22,  1., AT_Glasses|AT_Hat        }, // 21 LEyebrowOuter
    { 16,  0, 23,  1., AT_Glasses|AT_Hat        }, // 22 LEyebrowTopOuter
    { 17, 22, 24,  1., AT_Glasses|AT_Hat        }, // 23 LEyebrowTopInner
    { 18, 23, 34,  1., AT_Glasses|AT_Hat        }, // 24 LEyebrowInner
    { 19, 24, 26,  1., AT_Glasses|AT_Hat        }, // 25 Point25
    { 20, 32, 27,  1., AT_Glasses|AT_Hat        }, // 26 Point26
    { 32, 28, 30,  1., AT_Glasses|AT_Eye|AT_Hat }, // 27 LEyeOuter
    { 33, 27, 29,  1., AT_Glasses|AT_Eye|AT_Hat }, // 28 LEyeTop
    { 34,  1, 13,  1., AT_Glasses|AT_Eye|AT_Hat }, // 29 LEyeInner
    { 35, 27, 29,  1., AT_Glasses|AT_Eye|AT_Hat }, // 30 LEyeBot
    { 36, 27, 29,  1., AT_Glasses|AT_Eye|AT_Hat }, // 31 LPupil
    { 27, 33, 35,  1., AT_Glasses|AT_Eye|AT_Hat }, // 32 REyeOuter
    { 28, 32, 34,  1., AT_Glasses|AT_Eye|AT_Hat }, // 33 REyeTop
    { 29, 13,  1,  1., AT_Glasses|AT_Eye|AT_Hat }, // 34 REyeInner
    { 30, 32, 34,  1., AT_Glasses|AT_Eye|AT_Hat }, // 35 REyeBot
    { 31, 32, 34,  1., AT_Glasses|AT_Eye|AT_Hat }, // 36 RPupil
    { 45, 27, 51,  1., AT_Glasses|AT_Hat        }, // 37 LNoseTop
    { 44, -1, -1,  1., AT_Glasses|AT_Hat        }, // 38 LNoseMid
    { 43, -1, -1,  1., AT_Mustache|AT_Hat       }, // 39 LNostrilBot0
    { 42, -1, -1,  1., AT_Mustache|AT_Hat       }, // 40 LNostrilBot1
    { -1, 40, 42,  1., AT_Mustache|AT_Hat       }, // 41 CNoseBase
    { 40, -1, -1,  1., AT_Mustache|AT_Hat       }, // 42 RNoseBot1
    { 39, -1, -1,  1., AT_Mustache|AT_Hat       }, // 43 RNoseBot0
    { 38, -1, -1,  1., AT_Glasses|AT_Hat        }, // 44 MRNoseMid
    { 37, 32, 51,  1., AT_Glasses|AT_Hat        }, // 45 RNoseTop
    { 47, 31, 47,  1., AT_Mustache|AT_Hat       }, // 46 LNostril
    { 46, 36, 46,  1., AT_Mustache|AT_Hat       }, // 47 RNostril
    { 54, 51, 57,  1., AT_Mustache|AT_Hat       }, // 48 LMouthCorner
    { 53, -1, -1,  1., AT_Mustache|AT_Hat       }, // 49 Mouth49
    { 52, -1, -1,  1., AT_Mustache|AT_Hat       }, // 50 Mouth50
    { -1, -1, -1,  1., AT_Mustache|AT_Hat       }, // 51 TopOfTopLip
    { 50, -1, -1,  1., AT_Mustache|AT_Hat       }, // 52 Mouth52
    { 49, -1, -1,  1., AT_Mustache|AT_Hat       }, // 53 Mouth53
    { 48, 51, 57,  1., AT_Mustache|AT_Hat       }, // 54 RMouthCorner
    { 59, 54, 56,  1., 0|AT_Hat                 }, // 55 Mouth55
    { 58, -1, -1,  1., 0|AT_Hat                 }, // 56 Mouth56
    { -1, -1, -1,  1., 0|AT_Hat                 }, // 57 MouthBotOfBotLip
    { 56, -1, -1,  1., 0|AT_Hat                 }, // 58 Mouth58
    { 55, 48, 58,  1., 0|AT_Hat                 }, // 59 Mouth59
    { 62, 48, 61,  1., 0|AT_Hat                 }, // 60 Mouth60
    { -1, 60, 62,  1., 0|AT_Hat                 }, // 61 Mouth61
    { 60, 54, 61,  1., 0|AT_Hat                 }, // 62 Mouth62
    { 65, 48, 54,  1., 0|AT_Hat                 }, // 63 Mouth63
    { -1, 63, 65,  1., 0|AT_Hat                 }, // 64 Mouth64
    { 63, 48, 54,  1., 0|AT_Hat                 }, // 65 Mouth65
    { -1, 48, 54,  1., 0|AT_Hat                 }, // 66 MouthCenter
    { -1, 48, 54,  1., 0|AT_Hat                 }, // 67 NoseTip
};

#endif // STASM_LANDTAB_MUCT68_H
