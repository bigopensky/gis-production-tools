// ========================================================================
// rsmc-ortho  -- Calculate the ortho-rectification version of a plain
// tiff image by given set of mapping points and a new image size.
// ========================================================================
// Author: Alexander Weidauer <alex.weidauer@ifgdv.de>
// Created: 2021-09-10
// Version: 1.0
// ========================================================================
// (C) 2021 Alexander Weidauer  <alex.weidauer@ifgdv.de>
// All Rights Reserved
//
// The code is released under 3-clause BSD Licence 
// ========================================================================
// Changelog:
//   DATE        WHO   ITEM
//   2021-09-10  i4w   Initial setup
//   2021-09-12  i4w   CLI-Pasrer from Test static to the CLI interface
//   2021-09-13  i4w   Add CLI-Help
//   2021-09-29  i4w   Implement image size parameter
//   2021-09-30  i4w   Code documentation
//   2021-10-04  i4w   Change the program name to rsmc-ortho
//   2021-10-11  i4w   Simplify file management
// =====================================================================
// TODO:
//   - Complete code documentation, expand to full doxygen code
//   - separate the CLI parser from the main code int cli.(hpp,cpp)
//   - Create a man page
// =====================================================================
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <iostream>
#include <boost/filesystem.hpp>
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

//+ NAMESPACES =============================================================
using namespace std;
using namespace cv;
namespace fs = boost::filesystem;

//+ DEFINITIONS ============================================================
#define NO_ARG 0
#define REQ_ARG 1
#define OPT_ARG 2
#define NIL nullptr

#define VERSION "1.0"
#define PROGRAM "rsmc-ortho"
#define TITLE   "RSMC Ortho-Rectification via OpenCV"

//+ DEBUG ONLY
const int CHECK_ARGC = 0;

//+ TYPES ==================================================================
typedef Point2f TPoint;
typedef vector<TPoint> TPoints;
typedef Mat    TMatrix;
typedef string TString;
typedef fs::path TPath;
typedef int    TInteger;

struct TBBox {
  double xmin;
  double xmax;
  double ymin;
  double ymax;
};

// =========================================================================
//+ SERVICE ROUTINES
// =========================================================================

void printHelp() {
  cout <<
    PROGRAM "\\\n"
    "        -B <MIN.X> <MIN.Y> <MAX.X> <MAX.Y> \\\n"
    "        -I <INCOMING-IMAGE> \\\n"
    "        -R <RECTIFIED-IMAGE> \\\n"
    "        -S <WIDTH> <HEIGHT>\\\n"
    "        -O  <X1> <Y1> .. <XN> <YN>\\\n"
    "        -M  <X1> <Y1> .. <XN> <YN>\n\n"
    "  -B  --bbox <MIN.X> <MIN.Y> <MAX.X> <MAX.Y>: Image bounding box [m]\n"
    "        <MIN.X <MIN.Y> minimum coordinates of numeric type\n"
    "        <MAX.X <MAX.Y> maximum coordinates of numeric type\n"
    "\n"
    "  -I  --incoming-image  <FILE-NAME>: incoming image \n\n"
    "  -R  --rectified-image <FILE-NAME>: Name of the produced file.\n"
    "\n"
    "  -M  --mapped-icps <X1> <Y1> .. <XN> <YN>: vector of image control\n"
    "        points (ICPS) are N mapped perspective coordinates XY in the\n"
    "        image space. The tuples <Xn> & <Yn> have a numeric type\n"
    "\n"
    "  -O  --original-icps <X1> <Y1> .. <XN> <YN> vector of image control\n"
    "        points (ICPS) are N original coordinates corresponding to the \n"
    "        mapped ICPS in the image space and have a type numeric\n"
    "\n"
    "  -S  --image-size <WIDTH> <HEIGHT>: Width & height of the resulting image.\n"
    "        <WIDTH> & <HEIGHT> have a integer type.      "
    "\n"
    "  -h  --help:          Show help\n"
    "\n"
    "  -v  --version:       Show version\n"
    "\n";
}

// -------------------------------------------------------------------------
/// \brief Print the program version
///
///
void printVersion(const char* name) {
  cout << "PROGRAM: " << name << " VERSION: " VERSION "\n"
  "DESCRIPTION: " TITLE "\n\n";
}

// -------------------------------------------------------------------------
/// \brief Checks if the next char in a null terminated char is a whitespace
///
/// aka String is empty
bool isSpace(const char* s) {
  size_t i, len;
  if(s && *s) {
       len = strlen(s);
       for(i=0; i< len; i++)
          if (! isspace(s[i]) ) return false;
   }
   return true;
}

// -------------------------------------------------------------------------
/// \brief Get one CLI argument and increase to read counter
///
/// The function tries to read on commandline argument from a defined
/// tuple of rgument counts, argument data for a given parameter tag and
/// increases the argument counter.
///
/// The function succeeds or stops the program with an error message.
///
/// \param argCount - number of arguments in the incoming CLI set
/// \param args     - array of null terminated strings containg the data
/// \param argPos   - current position in the incoming argument array
///                   and outgoing new postion after a call
/// \param tag      - parameter tag to read (info only)
///
/// \returns a constant pointer to the to the decoded string

const char* getArg(int argCount, const char **args, int &argPos,
                     const TString tag) {
  if (argPos >= argCount) {
      cerr << "ERROR: Insufficient number of CLI parameter for "
                << "parameter '" << tag <<"'!\n";
      exit(255);
  }
  const char *res = args[argPos]; argPos++;
  if ( ( res == NIL ) ||
       (strlen(res) == 0) ||
       isSpace(res) ) {
     cerr << "ERROR: Empty value for parameter '" << tag <<"'!\n";
     exit(255);
   }
  return res;
}

// -------------------------------------------------------------------------
/// \brief Checks the number of available CLI arguments

void checkArgCount(int argCount, int argPos,
                   const TString arg, int argMore) {
  if (argPos+argMore > argCount) {
     cerr << "ERROR: Insufficient number of CLI parameter for "
               << "parameter '" << arg <<"'!\n"
               << "NOTE: At least " << argMore
               << "parameter are required!\n";
     exit(255);
  }
}

// -------------------------------------------------------------------------
/// \brief Checks if a given path exists

void checkPathExists(const TPath aPath,
                     const TString context) {
  if ( !fs::is_directory(aPath) ) {
      cerr << "ERROR: The " << context << " path '"
           << aPath << "' does not exsits!";
      exit(255);
  }
}

// -------------------------------------------------------------------------
/// \brief Checks if a given file exists

void checkFileExists(const TString strFile,
                     const TString context) {
  if ( !fs::exists(strFile) ) {
      cerr << "ERROR: The " << context << " file '"
           << strFile << "' does not exsits!";
      exit(255);
  }
}

// -------------------------------------------------------------------------
/// \brief Checks if a parsed parameter was found and initialized by the CLI

void checkParamInit(bool ok, const TString param) {
  if ( ! ok ) {
      cerr << "ERROR: Missing parameter --" << param << "!\n"
           << "NOTE: Call " << PROGRAM << " --help for furhter infos!\n";
      exit(255);
  }
}

// -------------------------------------------------------------------------
/// \brief Checks if a given parameter is a long or short annotated.

bool isArgParam(const char *arg) {
   size_t len =  strlen(arg);
   if ( (len>1) &&
        (arg[0]=='-') &&
        isalpha(arg[1]) )
        return true;
   if ( (len>2) &&
        (arg[0]=='-') &&
        (arg[1]=='-') &&
        isalpha(arg[2]) )
        return true;
  return false;
}

// -------------------------------------------------------------------------
/// \brief Get a CLI parameter value as char*

const char* getArgStr(int argCount, const char **args,
                      int &argPos, const TString tag) {
   const char* res = getArg(argCount, args, argPos, tag);
   if (isArgParam(res)) {
       cerr << "ERROR: Value expected for parameter'" << tag << "', but "
            << "parameter '" << res << " found!\n";
       exit(255);
     }
   return res;
}

// -------------------------------------------------------------------------
/// \brief Get a CLI parameter value as integer

int getArgInt(int argCount, const char **args,
              int &argPos, const TString tag) {
  int res = 0;
  const char *arg = getArgStr(argCount, args, argPos, tag);
  if (sscanf(arg, "%d", &res) ==0) {
      cerr << "ERROR: Invalid integer value '"
           << arg << "' for '" << tag <<"'!\n";
      exit(255);
    }
  return res;
}

// -------------------------------------------------------------------------
/// \brief Get a CLI parameter value as double

double getArgReal(int argCount, const char **args,
                  int &argPos, const TString tag) {
  double res = 0;
  const char *arg = getArgStr(argCount, args, argPos, tag);
  if (sscanf(arg, "%lf", &res) == 0) {
      cerr << "ERROR: Invalid real parameter value '"
           << arg << "' for '" << tag <<"'!\n";
      exit(255);
    }
  return res;
}

// -------------------------------------------------------------------------
/// \brief Get a CLI parameter set value as point list char*

void getArgPoints(int argCount, const char **args,
                         int &argPos, const TString tag,
                         TPoints &list) {
   int cntPoint = 0;
   const char* arg = getArg(argCount, args, argPos, tag);
   while ( !( isArgParam(arg) || (argPos >= argCount)) ) {
       float x = 0.0;
       float y = 0.0;
       cntPoint++;
       if (sscanf(arg, "%f", &x) ==0) {
           cerr << "ERROR: Invalid real value for X["<< cntPoint << "] = '"
                << arg << "' for parameter '" << tag <<"'!\n";
           exit(255);
        }
        arg = getArg(argCount, args, argPos, tag);
        if (sscanf(arg, "%f", &y) ==0) {
            cerr << "ERROR: Invalid real value for Y["<< cntPoint << "] = '"
                 << arg << "' for parameter '" << tag <<"'!\n";
            exit(255);
         }
        const TPoint p(x,y);
        list.push_back(p);
        if ( argPos < argCount) {
           arg = getArg(argCount, args, argPos, tag);
        } else { argPos++; }
     }
     if (argPos < argCount) argPos--;
}

// -------------------------------------------------------------------------
/// \brief Read and parse the CLI parameter set for the app

void readCli(int argc, const char** argv,
                 TString  &inImage,
                 TString  &outImage,
                 TBBox    &ImageBBox,
                 TInteger &ImageWidth,
                 TInteger &ImageHeight,
                 TPoints  &IcpsOriginal,
                 TPoints  &IcpsMapped) {

   // current argument position
   int argPos = 0;

   // Check if we have enough params in the data array
   if (CHECK_ARGC && ( argc < 30 )) {
         cerr
           << "ERROR: In sufficient number of CLI parameter present!\n"
           << "NOTE: Call " PROGRAM  " --help for further  infos\n";
   }
   // Iterate over args
   while (argPos< argc) {

       TString arg(argv[argPos]); argPos++;

       // if (DEBUG) { cout << argPos << ":" << argv[argPos] << "\n"; }

       // Skip program name -------------------------------------------------
       if (argPos == 1) continue;

       // Parse bbox values -------------------------------------------------
       if ((arg == "-B") || (arg == "--bbox")) {
           checkArgCount(argc, argPos, arg, 4);
           ImageBBox.xmin = getArgReal(argc, argv, argPos, arg);
           ImageBBox.xmax = getArgReal(argc, argv, argPos, arg);
           ImageBBox.ymin = getArgReal(argc, argv, argPos, arg);
           ImageBBox.ymax = getArgReal(argc, argv, argPos, arg);
       }

       // Read capaign name ------------------------------------------------
       else if ((arg == "-I") || (arg == "--incoming-image")) {
           inImage = getArgStr(argc, argv, argPos, arg);
       }

       // Read campaign name ------------------------------------------------
       else if ((arg == "-R") || (arg == "--rectified-image")) {
           outImage = getArgStr(argc, argv, argPos, arg);
       }

       // Parse image width and height values ------------------------------
       else if ((arg == "-S") || (arg == "--image-size")) {
           checkArgCount(argc, argPos, arg, 2);
           ImageWidth  = getArgInt(argc, argv, argPos, arg);
           ImageHeight = getArgInt(argc, argv, argPos, arg);
       }

       // Read original points  --------------------------------------------
       else if ((arg == "-O") || (arg == "--original-icps")) {
           getArgPoints(argc, argv, argPos, arg, IcpsOriginal);
           // cout << IcpsOriginal << "\n" << IcpsOriginal.size() << "\n";
       }

       // Read mapped points -----------------------------------------------
       else if ((arg == "-M") || (arg == "--mapped-icps")) {
           getArgPoints(argc, argv, argPos, arg, IcpsMapped);
           // cout << IcpsMapped << "\n";
       }

       // Print help -------------------------------------------------------
       else if ((arg == "-h") || (arg == "--help")) {
           printHelp();
           exit(1);
       }

       // Print Version ----------------------------------------------------
       else if ((arg == "-v") || (arg == "--version")) {
           printVersion(argv[0]);
           exit(1);
       }
       // CLI parameter is unknown -----------------------------------------
       else {
         cerr << "ERROR: Unknown CLI parameter '" << arg << "'!\n\n";
         exit(255);
       }
     }
       // ==== ASSERTATIONS =================================================

       checkParamInit(inImage   != "", "incoming-image");
       checkParamInit(outImage  != "", "rectified-image");

       checkParamInit( ! ( (ImageBBox.xmin == 0.0) ||
                           (ImageBBox.xmax == 0.0) ||
                           (ImageBBox.ymin == 0.0) ||
                           (ImageBBox.ymax == 0.0) ),
                        "bbox");

       checkParamInit(  (ImageWidth > 0) && (ImageHeight > 0) ,
                       "image-size");

       checkParamInit( IcpsMapped.size() > 3   , "mapped-icsp");
       checkParamInit( IcpsOriginal.size() > 3 , "original-icsp");

       if ( IcpsMapped.size() != IcpsOriginal.size() ) {
           cerr << "Lists length of mapped ICPS and orgiginal " <<
                   "ICPS is not equal!\n";
           exit(255);
       }
       checkPathExists(TPath(outImage).parent_path(),"work path");
       checkFileExists(inImage,"incoming image");
}

// =========================================================================
//+ MAIN ROUTINE
// =========================================================================
int main(int argc, const char* argv[]) {

  // Init needed variables -------------------------------------------------
  TPoints  icpsOriginal;
  TPoints  icpsMapped;
  TString  camSerial = "";
  TString  inImage   = "";
  TString  outImage  = "";
  TInteger imgWidth  = -1;
  TInteger imgHeight = -1;
  TBBox    imgBBox   = { 0., 0., 0., 0.};

  // Parse the CLI parameter set -------------------------------------------
  cout << ".READ CLI\n";
  readCli(argc, argv,
          inImage, outImage,
          imgBBox, imgWidth, imgHeight,
          icpsOriginal, icpsMapped);

  // Read the image file ---------------------------------------------------
  cout << ".READ INPUT " << inImage << "\n";
  TMatrix imgOriginal = cv::imread(inImage);

  // Calculate the perspective ---------------------------------------------
  cout << ".CALC PERSPECTIVE" << outImage << "\n";
  TMatrix trfmPerpective = getPerspectiveTransform(icpsOriginal,
                                                   icpsMapped);

  // Orthorectify the image ------------------------------------------------
  TMatrix imgMapped;
  cout << ".MAP  PERSPECTIVE\n";
  warpPerspective(imgOriginal,
                  imgMapped,
                  trfmPerpective,
                  Size(imgWidth, imgHeight), //imgOriginal.size(),
                  cv::INTER_LANCZOS4);

  // Write the result ------------------------------------------------------
  cout << ".WRITE MAPPED " << outImage << "\n";
  imwrite(outImage, imgMapped);
  cout << ".ORTHO CALCULATUION OK\n\n";
  exit(0);
}
// ========================================================================
//+ EOF
// ========================================================================
