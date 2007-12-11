/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include <QtCore/QString>

#include "AppConfig.h"
#include "DataFusionTests.h"
#include "Classification.h"
#include "AppVerify.h"
#include "ConfigurationSettings.h"
#include "DataFusion.h"
#include "DataFusionTools.h"
#include "DateTime.h"
#include "DimensionDescriptor.h"
#include "ObjectFactory.h"
#include "ObjectResource.h"
#include "Poly2D.h"
#include "Polywarp.h"
#include "Progress.h"
#include "ProgressTracker.h"
#include "RasterElement.h"
#include "RasterDataDescriptor.h"
#include "TestDataPath.h"

#include <iostream>
#include <fstream>
#include <string>
#if defined(UNIX_API)
#include <unistd.h>
#endif

// don't sleep on Windows during testing
#if defined(WIN_API)
#define sleep(x)
#endif

using namespace std;

const double SMALL_VALUE = 1e-4;

bool compareDatesFunc(ostream& stream, const char* date1Name, const char* date2Name,
                     const DateTime* pDate1, const DateTime* pDate2)
{
   if (pDate1->getStructured() != pDate2->getStructured())
   {
      stream << "Dates " + string(date1Name) + " and " + string(date2Name) + " are not equal!" << endl;
      return false;
   }
   return true;
}

#define compareDates(stream, date1, date2) compareDatesFunc(stream, #date1, #date2, date1, date2)

// begin PolywarpTests
PolywarpTests::PolywarpTests(ostream& output, ProgressTracker &tracker) : Test(output, tracker)
{
   myStage = ProgressTracker::Stage("Polywarp Tests", "app",
                                    "7EC610DE-0CC6-4a75-B65D-CAB022706268", 100);
   mStages.push_back(ProgressTracker::Stage("Positive Shift Test", "app",
                                            "E32556B9-4590-4670-B5E1-4BE6BEEB10D9", 20));
   mStages.push_back(ProgressTracker::Stage("Negative Shift Test", "app",
                                            "CF284597-EE1F-412d-9E7C-E10A313B0AA6", 20));
   mStages.push_back(ProgressTracker::Stage("Positive Shift and Scale Test", "app",
                                            "9086D23D-D7F6-4aff-847D-47AE787731E4", 20));
   mStages.push_back(ProgressTracker::Stage("Negative Shift and Scale Test", "app",
                                            "B9C9569C-BFCD-4f5c-BD4F-58B6E4063D26", 20));
   mStages.push_back(ProgressTracker::Stage("Variable Shift Test", "app",
                                            "76F00BAE-E721-41b8-8E76-6C8D324D4ADA", 20));   
}

bool PolywarpTests::run(double pause)
{
   bool bSuccess = true;

   ProgressSubdivision division(&mProgressTracker, mStages);
   bSuccess = positiveShiftTest();
   if (bSuccess)
   {
      sleep(pause);
      bSuccess = negativeShiftTest();
   }
   if (bSuccess)
   {
      sleep(pause);
      bSuccess = positiveShiftAndScaleTest();
   }
   if (bSuccess)
   {
      sleep(pause);
      bSuccess = negativeShiftAndScaleTest();
   }
   if (bSuccess)
   {
      sleep(pause);
      bSuccess = varyXShiftTest();
   }
   sleep(pause);
   return bSuccess;
}

void PolywarpTests::setupInputMatrices(Vector<double>& XP, Vector<double>& YP,
                                       Vector<double>& XS, Vector<double>& YS,
                                       Vector<double>& KX, Vector<double>& KY,
                                       Vector<double>& ExpectedKX, Vector<double>& ExpectedKY)
{
   //Polywarp input declarations
   XP = YP = XS = YS = Vector<double>(4);
   KX = KY = ExpectedKX = ExpectedKY = Vector<double>(4);

   XP[0] = 36.0;
   XP[1] = 36.0;
   XP[2] = 53.0;
   XP[3] = 53.0;

   YP[0] = 2.0;
   YP[1] = 25.0;
   YP[2] = 25.0;
   YP[3] = 2.0;
}

bool PolywarpTests::verifyVector(ProgressTracker::Stage& s, const Vector<double>& results, std::string name)
{
   unsigned int i = 0;
   bool bAllGood = true;
   int workDone = 0;
   for (i = 0; i < results.size(); i++)
   {
      workDone = int((i+1)*100/double(results.size()));
      if (fabs(results.at(i)) > SMALL_VALUE)
      {
         string msg = name + " Failed! " + "Value[" + QString::number(i).toStdString() +
                             "] = " + QString::number(fabs(results[i])).toStdString() +
                             " and should be smaller than " + QString::number(SMALL_VALUE).toStdString();
         mOutputStream << msg << endl;
         mProgressTracker.report(msg, workDone, WARNING);

         bAllGood = false;
      }
      else
      {
         mProgressTracker.report("Verifying Vectors...", workDone, NORMAL);
      }
   }
   return bAllGood;
}

bool PolywarpTests::positiveShiftTest()
{
   bool bSuccess = true;

   // POLYWARP TEST 1 - Constant Positive Shift
   Vector<double> XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY;
   setupInputMatrices(XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY);

   double xshift = 5.0, yshift = 3.0;
   
   XS[0] = XP[0] + xshift;
   XS[1] = XP[1] + xshift; 
   XS[2] = XP[2] + xshift;
   XS[3] = XP[3] + xshift;
   YS[0] = YP[0] + yshift;
   YS[1] = YP[1] + yshift; 
   YS[2] = YP[2] + yshift;
   YS[3] = YP[3] + yshift;
   
   ExpectedKX[0] = xshift;
   ExpectedKX[1] = 0.0;
   ExpectedKX[2] = 1.0;
   ExpectedKX[3] = 0.0;

   ExpectedKY[0] = yshift;
   ExpectedKY[1] = 1.0;
   ExpectedKY[2] = 0.0;
   ExpectedKY[3] = 0.0;

   // int workIncrement = (maxWork-minWork)/2;
   polywarp(XS, YS, XP, YP, KX, KY, 1, mProgressTracker);

   Vector<double> resultsX = KX-ExpectedKX;
   Vector<double> resultsY = KY-ExpectedKY;

   bSuccess = verifyVector(myStage.getActiveStage(), resultsX, "Polywarp-1: X Results");
   if (bSuccess)
   {
      bSuccess = verifyVector(myStage.getActiveStage(), resultsY, "Polywarp-1: Y Results");
   }

   mProgressTracker.nextStage();
   return bSuccess;
}

bool PolywarpTests::negativeShiftTest()
{
   bool bSuccess = true;

   //POLYWARP TEST 2 - Constant Negative Shift
   Vector<double> XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY;
   setupInputMatrices(XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY);

   double xshift = -5.25, yshift = -3.3;

   XS[0] = XP[0] + xshift;
   XS[1] = XP[1] + xshift; 
   XS[2] = XP[2] + xshift;
   XS[3] = XP[3] + xshift;
   YS[0] = YP[0] + yshift;
   YS[1] = YP[1] + yshift; 
   YS[2] = YP[2] + yshift;
   YS[3] = YP[3] + yshift;

   ExpectedKX[0] = xshift;
   ExpectedKX[1] = 0.0;
   ExpectedKX[2] = 1.0;
   ExpectedKX[3] = 0.0;

   ExpectedKY[0] = yshift;
   ExpectedKY[1] = 1.0;
   ExpectedKY[2] = 0.0;
   ExpectedKY[3] = 0.0;

   polywarp (XS, YS, XP, YP, KX, KY, 1, mProgressTracker);

   Vector<double> resultsX = KX-ExpectedKX;
   Vector<double> resultsY = KY-ExpectedKY;

   bSuccess = verifyVector(myStage.getActiveStage(), resultsX, "Polywarp-2: X Results");
   if (bSuccess)
   {
      bSuccess = verifyVector(myStage.getActiveStage(), resultsY, "Polywarp-2: Y Results");
   }

   mProgressTracker.nextStage();
   return bSuccess;
}

bool PolywarpTests::positiveShiftAndScaleTest()
{
   bool bSuccess = true;

   //POLYWARP TEST 3 - Constant Positive Shift and Scale
   Vector<double> XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY;
   setupInputMatrices(XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY);

   double xshift = 2.0, yshift = 6.3, xscale = 2.0, yscale = 3.5;

   XS[0] = XP[0]*xscale + xshift;
   XS[1] = XP[1]*xscale + xshift; 
   XS[2] = XP[2]*xscale + xshift;
   XS[3] = XP[3]*xscale + xshift;
   YS[0] = YP[0]*yscale + yshift;
   YS[1] = YP[1]*yscale + yshift; 
   YS[2] = YP[2]*yscale + yshift;
   YS[3] = YP[3]*yscale + yshift;

   ExpectedKX[0] = xshift;
   ExpectedKX[1] = 0.0;
   ExpectedKX[2] = xscale;
   ExpectedKX[3] = 0.0;

   ExpectedKY[0] = yshift;
   ExpectedKY[1] = yscale;
   ExpectedKY[2] = 0.0;
   ExpectedKY[3] = 0.0;

   polywarp (XS, YS, XP, YP, KX, KY, 1, mProgressTracker);

   Vector<double> resultsX = KX-ExpectedKX;
   Vector<double> resultsY = KY-ExpectedKY;

   bSuccess = verifyVector(myStage.getActiveStage(), resultsX, "Polywarp-3: X Results");
   if (bSuccess)
   {
      bSuccess = verifyVector(myStage.getActiveStage(), resultsY, "Polywarp-3: Y Results");
   }

   mProgressTracker.nextStage();
   return bSuccess;
}

bool PolywarpTests::negativeShiftAndScaleTest()
{
   bool bSuccess = true;

   //POLYWARP TEST 4 - Constant Negative Shift and Scale
   Vector<double> XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY;
   setupInputMatrices(XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY);

   double xshift = -2.0, yshift = -6.3, xscale = -2.0, yscale = -3.5;

   XS[0] = XP[0]*xscale + xshift;
   XS[1] = XP[1]*xscale + xshift; 
   XS[2] = XP[2]*xscale + xshift;
   XS[3] = XP[3]*xscale + xshift;
   YS[0] = YP[0]*yscale + yshift;
   YS[1] = YP[1]*yscale + yshift; 
   YS[2] = YP[2]*yscale + yshift;
   YS[3] = YP[3]*yscale + yshift;

   ExpectedKX[0] = xshift;
   ExpectedKX[1] = 0.0;
   ExpectedKX[2] = xscale;
   ExpectedKX[3] = 0.0;
   
   ExpectedKY[0] = yshift;
   ExpectedKY[1] = yscale;
   ExpectedKY[2] = 0.0;
   ExpectedKY[3] = 0.0;

   polywarp (XS, YS, XP, YP, KX, KY, 1, mProgressTracker);

   Vector<double> resultsX = KX-ExpectedKX;
   Vector<double> resultsY = KY-ExpectedKY;

   bSuccess = verifyVector(myStage.getActiveStage(), resultsX, "Polywarp-4: X Results");
   if (bSuccess)
   {
      bSuccess = verifyVector(myStage.getActiveStage(), resultsY, "Polywarp-4: Y Results");
   }

   mProgressTracker.nextStage();
   return bSuccess;
}

bool PolywarpTests::varyXShiftTest()
{
   bool bSuccess = true;

   //POLYWARP TEST 5 - Varying X Shift
   Vector<double> XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY;
   setupInputMatrices(XP, YP, XS, YS, KX, KY, ExpectedKX, ExpectedKY);

   XS[0] = XP[0];
   XS[1] = XP[1] + 2.0; 
   XS[2] = XP[2] + 2.0;
   XS[3] = XP[3];
   YS[0] = YP[0] + 2.0;
   YS[1] = YP[1] + 2.0; 
   YS[2] = YP[2];
   YS[3] = YP[3] - 2.0;

   ExpectedKX[0] = -0.173913;
   ExpectedKX[1] = 0.0869565;
   ExpectedKX[2] = 1.0;
   ExpectedKX[3] = 0.0;

   ExpectedKY[0] = 10.8389;
   ExpectedKY[1] = 0.815857;
   ExpectedKY[2] = -0.245524;
   ExpectedKY[3] = 0.00511509;

   polywarp (XS, YS, XP, YP, KX, KY, 1, mProgressTracker);

   Vector<double> resultsX = KX-ExpectedKX;
   Vector<double> resultsY = KY-ExpectedKY;

   bSuccess = verifyVector(myStage.getActiveStage(), resultsX, "Polywarp-5: X Results");
   if (bSuccess)
   {
      bSuccess = verifyVector(myStage.getActiveStage(), resultsY, "Polywarp-5: Y Results");
   }

   mProgressTracker.nextStage();
   return bSuccess;
}

// begin Poly2DTests
Poly2DTests::Poly2DTests(ostream& output, ProgressTracker &tracker) : Test(output, tracker)
{
   myStage = ProgressTracker::Stage("Poly2D Tests", "app",
                                    "DB0ADBB4-05A9-4d89-9BE9-761E160271EC", 100);

   mStages.push_back(ProgressTracker::Stage("Identity Test", "app",
                                            "ABFE1841-CD8F-4b26-80B5-F18F4593F3A7", 20));
   mStages.push_back(ProgressTracker::Stage("Positive Shift Test", "app",
                                            "B04EDF2C-58F4-432e-AEEE-E9E1940C0049", 20));
   mStages.push_back(ProgressTracker::Stage("Positive Scale Test", "app",
                                            "E1758988-7DB9-4efe-AA73-A98F3DA35A01", 20));
   mStages.push_back(ProgressTracker::Stage("Positive Shift And Scale Test", "app",
                                            "C4D0A1F5-4040-4bb4-AE14-0FCC3DA4E081", 20));
}

bool Poly2DTests::run(double pause)
{
   bool bSuccess = true;

   ProgressSubdivision division(&mProgressTracker, mStages);
   bSuccess = identityTest();
   if (bSuccess)
   {
      sleep(pause);
      bSuccess = positiveShiftTest();
   }
   if (bSuccess)
   {
      sleep(pause);
      bSuccess = positiveScaleTest();
   }
   if (bSuccess)
   {
      sleep(pause);
      bSuccess = positiveShiftAndScaleTest();
   }
   sleep(pause);
   return bSuccess;
}

bool Poly2DTests::runTest(std::string inputFile, std::string outputFile, std::string testName,
                          const Vector<double>& KX, const Vector<double>& KY,
                          unsigned int nx, unsigned int ny, unsigned int newx, unsigned int newy)
{
   string fullPath = getTestDataPath() + "/DataFusion/";
   inputFile = fullPath + inputFile;

   Service<ModelServices> pModel;
   VERIFY(pModel.get() != NULL);

   FactoryResource<Classification> pClass;
   VERIFY(pClass.get() != NULL);

   FactoryResource<DateTime> pDeClassDate;
   VERIFY(pDeClassDate.get() != NULL);
   FactoryResource<DateTime> pDowngradeDate;
   VERIFY(pDowngradeDate.get() != NULL);
   FactoryResource<DateTime> pSecuritySrcDate;
   VERIFY(pSecuritySrcDate.get() != NULL);

   VERIFY(pDeClassDate->set(1941, 12, 7, 5, 29, 03) == true);
   VERIFY(pDeClassDate->isValid() == true);
   // this should be an invalid date because 2015 is not a leap year
   VERIFY(pDowngradeDate->set(2015, 2, 29, 12, 13, 14) != true);
   VERIFY(pDowngradeDate->isValid() != true);
   // this should be a valid date because 2016 is a leap year
   VERIFY(pDowngradeDate->set(2016, 2, 29, 12, 13, 14) == true);
   VERIFY(pDowngradeDate->isValid() == true);
   VERIFY(pSecuritySrcDate->set(1970, 3, 4, 5, 58, 59) == true);
   VERIFY(pSecuritySrcDate->isValid() == true);

   const string S_LEVEL = "C";
   const string S_SYSTEM = "dummy-pc";
   const string S_CODEWORD = "DataFusionTest";
   const string S_FILECONTROL = "Computer";
   const string S_FILERELEASING = "Public";
   const string S_EXEMPTION = "none";
   const string S_COUNTRYCODE = "USA";
   const string S_DESCRIPTION = "This is a test description";
   const string S_AUTHORITY = "none needed";
   const string S_AUTHORITYTYPE = "O";
   const string S_SCN = "12345";
   const string S_FILECOPYNUMBER = "42";
   const string S_NUMCOPIES = "24";
   
   pClass->setLevel(S_LEVEL);
   pClass->setSystem(S_SYSTEM);
   pClass->setCodewords(S_CODEWORD);
   pClass->setFileControl(S_FILECONTROL);
   pClass->setFileReleasing(S_FILERELEASING);
   pClass->setDeclassificationExemption(S_EXEMPTION);
   pClass->setCountryCode(S_COUNTRYCODE);
   pClass->setDescription(S_DESCRIPTION);
   pClass->setAuthority(S_AUTHORITY);
   pClass->setAuthorityType(S_AUTHORITYTYPE);
   pClass->setSecurityControlNumber(S_SCN);
   pClass->setFileCopyNumber(S_FILECOPYNUMBER);
   pClass->setFileNumberOfCopies(S_NUMCOPIES);
   pClass->setDeclassificationDate(pDeClassDate.get());
   pClass->setDowngradeDate(pDowngradeDate.get());
   pClass->setSecuritySourceDate(pSecuritySrcDate.get());

   ModelResource<RasterElement> pInput(RasterUtilities::createRasterElement("InputMatrix", ny, nx, FLT8BYTES, true, NULL)); 
   VERIFY(pInput.get() != NULL);

   pInput->getDataDescriptor()->setClassification(pClass.get());

   FactoryResource<DataRequest> pRequest;
   pRequest->setWritable(true);

   unsigned int rowIndex = 0, colIndex = 0;


   ifstream resultsFile((fullPath+outputFile).c_str());

   if (!resultsFile.good())
   {
      string msg = outputFile + " does not exist or you do not have access!";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }

   // Scope the DataAccessor since it must be destroyed before the ModelResource is destroyed.
   {
      ifstream testFile(inputFile.c_str());

      if (testFile.eof())
      {
         string msg = inputFile + " does not exist or you do not have access!";
         mOutputStream << msg << endl;
         mProgressTracker.report(msg, 100, WARNING);
         return false;
      }

      DataAccessor rmda = pInput->getDataAccessor(pRequest.release());
      if (!rmda.isValid())
      {
         string msg = testName + ": Input matrix data accessor is invalid!";
         mOutputStream << msg << endl;
         mProgressTracker.report(msg, 100, WARNING);
         return false;
      }

      while (!testFile.eof() && rowIndex < ny)
      {
         while (!testFile.eof() && colIndex < nx)
         {
            string str;
            testFile >> str;
            static_cast<double*>(rmda->getRow())[colIndex++] = atof(str.c_str());
         }
         colIndex = 0;
         rmda->nextRow();
         rowIndex++;
      }
      testFile.close();
   }

   string msg = testName + ": Poly2d failed!";
   ModelResource<RasterElement> pOutput(reinterpret_cast<RasterElement*>(NULL));
   try
   {
      pOutput = ModelResource<RasterElement>(poly_2D<double>(
         NULL, pInput.get(), KX, KY, newx, newy, 0, 0, 1, mProgressTracker));
   }
   // If the operation fails due to an exception (bug/unrecoverable error), provide details
   catch (AssertException& exc)
   {
      msg = msg + " Cause: " + exc.getText();
   }
   catch (FusionException& exc)
   {
      msg = msg + " Cause: " + exc.toString();
   }

   if (pOutput.get() == NULL)
   {
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }

   const Classification *pOutputClass = pOutput->getClassification();
   if (pOutputClass == NULL)
   {
      string msg = testName + ": pOutput->getClassification() returned NULL!";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }

   if (pOutputClass->getLevel() != S_LEVEL)
   {
      string msg = testName + ": pOutputClass->getLevel() != S_LEVEL";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getSystem() != S_SYSTEM)
   {
      string msg = testName + ": pOutputClass->getSystem() != S_SYSTEM";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getCodewords() != S_CODEWORD)
   {
      string msg = testName + ": pOutputClass->getCodewords() != S_CODEWORD";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getFileControl() != S_FILECONTROL)
   {
      string msg = testName + ": pOutputClass->getFileControl() != S_FILECONTROL";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getFileReleasing() != S_FILERELEASING)
   {
      string msg = testName + ": pOutputClass->getFileReleasing() != S_FILERELEASING";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getDeclassificationExemption() != S_EXEMPTION)
   {
      string msg = testName + ": pOutputClass->getDeclassificationExemption() != S_EXEMPTION";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getCountryCode() != S_COUNTRYCODE)
   {
      string msg = testName + ": pOutputClass->getCountryCode() != S_COUNTRYCODE";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   } 
   if (pOutputClass->getDescription() != S_DESCRIPTION)
   {
      string msg = testName + ": pOutputClass->getDescription() != S_DESCRIPTION";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getAuthority() != S_AUTHORITY)
   {
      string msg = testName + ": pOutputClass->getAuthority() != S_AUTHORITY";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getAuthorityType() != S_AUTHORITYTYPE)
   {
      string msg = testName + ": pOutputClass->getAuthorityType() != S_AUTHORITYTYPE";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getSecurityControlNumber() != S_SCN)
   {
      string msg = testName + ": pOutputClass->getSecurityControlNumber() != S_SCN";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getFileCopyNumber() != S_FILECOPYNUMBER)
   {
      string msg = testName + ": pOutputClass->getFileCopyNumber() != S_FILECOPYNUMBER";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   if (pOutputClass->getFileNumberOfCopies() != S_NUMCOPIES)
   {
      string msg = testName + ": pOutputClass->getFileNumberOfCopies() != S_NUMCOPIES";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }
   const DateTime* pOutputDate = pOutputClass->getDeclassificationDate();
   if (pOutputDate == NULL || pDeClassDate.get() == NULL)
   {
      string msg = testName + ": pOutputClass->getDeclassificationDate() == NULL || pDeClassDate.get() == NULL";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }

   if (compareDates(mOutputStream, pOutputDate, pDeClassDate.get()) == false)
   {
      return false;
   }

   pOutputDate = pOutputClass->getDowngradeDate();
   if (pOutputDate == NULL || pDowngradeDate.get() == NULL)
   {
      string msg = testName + ": pOutputClass->getDowngradeDate() == NULL || pDowngradeDate.get() == NULL";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }

   if (compareDates(mOutputStream, pOutputDate, pDowngradeDate.get()) == false)
   {
      return false;
   }

   pOutputDate = pOutputClass->getSecuritySourceDate();
   if (pOutputDate == NULL || pSecuritySrcDate.get() == NULL)
   {
      string msg = testName + ": pOutputClass->getSecuritySourceDate() == NULL || pSecuritySrcDate.get() == NULL";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 100, WARNING);
      return false;
   }

   if (compareDates(mOutputStream, pOutputDate, pSecuritySrcDate.get()) == false)
   {
      return false;
   }

   ModelResource<RasterElement> pExpected(RasterUtilities::createRasterElement("ExpectedMatrix", newy, newx, FLT8BYTES));
   VERIFY(pExpected.get() != NULL);

   // Scope the DataAccessor since it must be destroyed before the ModelResource is destroyed.
   {
      DataAccessor rmda = pExpected->getDataAccessor();
      if (!rmda.isValid())
      {
         string msg = testName + ": Expected matrix data accessor is invalid!";
         mProgressTracker.report(msg, 100, WARNING);
         return false;
      }

      rowIndex = colIndex = 0;
      while (!resultsFile.eof() && rowIndex < newy)
      {
         while (!resultsFile.eof() && colIndex < newx)
         {
            string str;
            resultsFile >> str;
            static_cast<double*>(rmda->getRow())[colIndex++] = atof(str.c_str());
         }
         colIndex = 0;
         rmda->nextRow();
         rowIndex++;
      }

      resultsFile.close();
   }

   return verifyMatrix(myStage.getActiveStage(), pOutput.get(), pExpected.get(), testName);
}

bool Poly2DTests::verifyMatrix (ProgressTracker::Stage& s, RasterElement* pResults, RasterElement* pExpected,
                                std::string name)
{
   const double MAX_PCT_ERROR = 1.0;
   double mismatches = 0.0;

   VERIFY(pResults != NULL);
   VERIFY(pExpected != NULL);

   const RasterDataDescriptor* pDescExp = dynamic_cast<const RasterDataDescriptor*>(pExpected->getDataDescriptor());
   const RasterDataDescriptor* pDescRes = dynamic_cast<const RasterDataDescriptor*>(pResults->getDataDescriptor());
   
   VERIFY(pDescExp != NULL);
   VERIFY(pDescRes != NULL);

   unsigned int numRows = pDescRes->getRowCount();
   unsigned int numCols = pDescRes->getColumnCount();

   VERIFY(pDescExp->getRowCount() == pDescRes->getRowCount());
   VERIFY(pDescExp->getColumnCount() == pDescRes->getColumnCount());

   DataAccessor r = pResults->getDataAccessor();
   DataAccessor e = pExpected->getDataAccessor();

   for (unsigned int i = 0; i < pDescRes->getRowCount(); i++)
   {
      VERIFY(r.isValid());
      VERIFY(e.isValid());

      int workDone = int(double((i+1)*100)/pDescRes->getRowCount());

      double* rRow = static_cast<double*>(r->getRow());
      double* eRow = static_cast<double*>(e->getRow());

      string msg = "Verifying Results Matrices...";
      ReportingLevel lvl = NORMAL;

      for (unsigned int j = 0; j < pDescRes->getColumnCount(); j++)
      {
         double value = fabs(rRow[j]-eRow[j]);
         if (value > SMALL_VALUE)
         {
            mismatches++;
            msg = (name + " Failed! " + "Value[" +
                   QString::number(i).toStdString() + "][" +
                   QString::number(j).toStdString() + "] = " +
                   QString::number(value).toStdString() + " and should be smaller than " +
                   QString::number(SMALL_VALUE).toStdString());
            lvl = WARNING;
            mOutputStream << msg << endl;
            mProgressTracker.report(msg, workDone, lvl);
         }
      }
      mProgressTracker.report(msg, workDone, NORMAL);
      e->nextRow();
      r->nextRow();
   }

   double PercentError = 100*mismatches/(numRows*numCols);
   bool bSuccess = (PercentError < MAX_PCT_ERROR);

   if (!bSuccess)
   {
      string msg = name + " failed due to a high percent of error!";
      mOutputStream << msg << endl;
      mProgressTracker.report(msg, 0, WARNING);
   }
   return bSuccess;
}    

bool Poly2DTests::identityTest()
{
   Vector<double> KX(4), KY(4);

   //POLY2D TEST 1 - Identity 
   KX[0] = 1.0;
   KX[1] = 0.0;
   KX[2] = 1.0;
   KX[3] = 0.0;
   
   KY[0] = 1.0;
   KY[1] = 1.0;
   KY[2] = 0.0;
   KY[3] = 0.0;
   
   unsigned int newx = 4;
   unsigned int newy = 4;
   unsigned int nx = 10;
   unsigned int ny = 10;

   bool bSuccess =  runTest("IdentityTest.txt", "ExpectedIdentityResults.txt", "Poly2d-1",
                            KX, KY, nx, ny, newx, newy);
   mProgressTracker.nextStage();
   return bSuccess;
}

bool Poly2DTests::positiveShiftTest()
{
   Vector<double> KX(4), KY(4);

   //POLY2D TEST 2 - Constant Positive Shift
   KX[0] = 3.0;
   KX[1] = 0.0;
   KX[2] = 1.0;
   KX[3] = 0.0;

   KY[0] = 2.5;
   KY[1] = 1.0;
   KY[2] = 0.0;
   KY[3] = 0.0;

   unsigned int newx = 5;
   unsigned int newy = 5;
   unsigned int nx = 10;
   unsigned int ny = 10;

   bool bSuccess =  runTest("ShiftTest.txt", "ExpectedShiftResults.txt", "Poly2d-2",
                            KX, KY, nx, ny, newx, newy);
   mProgressTracker.nextStage();
   return bSuccess;   
}

bool Poly2DTests::positiveScaleTest()
{
   Vector<double> KX(4), KY(4);

   //POLY2D TEST 3 - Constant Positive Scale
   KX[0] = 3.0;
   KX[1] = 0.0;
   KX[2] = 1.0;
   KX[3] = 0.0;

   KY[0] = 2.5;
   KY[1] = 1.0;
   KY[2] = 0.0;
   KY[3] = 0.0;

   unsigned int newx = 5;
   unsigned int newy = 5;
   unsigned int nx = 10;
   unsigned int ny = 10;

   bool bSuccess =  runTest("ScaleTest.txt", "ExpectedScaleResults.txt", "Poly2d-3",
                            KX, KY, nx, ny, newx, newy);
   mProgressTracker.nextStage();
   return bSuccess;   
}

bool Poly2DTests::positiveShiftAndScaleTest()
{
   Vector<double> KX(4), KY(4);

   //POLY2D TEST 4 - Constant Positive Shift and Scale
   KX[0] = 3.0;
   KX[1] = 0.0;
   KX[2] = 1.0;
   KX[3] = 0.0;

   KY[0] = 2.5;
   KY[1] = 1.0; 
   KY[2] = 0.0;
   KY[3] = 0.0;

   unsigned int newx = 9;
   unsigned int newy = 9;
   unsigned int nx = 10;
   unsigned int ny = 10;

   bool bSuccess =  runTest("ShiftandScaleTest.txt", "ExpectedShiftandScaleResults.txt", "Poly2d-4",
                            KX, KY, nx, ny, newx, newy);
   mProgressTracker.nextStage();
   return bSuccess;   
}

bool DataFusion::runOperationalTests(Progress *progress, std::ostream& failure)
{
   return true;
}

bool DataFusion::runAllTests(Progress *pProgress, std::ostream& failure)
{
   bool bSuccess = true;
   vector<ProgressTracker::Stage> vStages;
   
   PolywarpTests pwarpTests(failure, mProgressTracker);
   Poly2DTests p2dTests(failure, mProgressTracker);
   vStages.push_back(pwarpTests.getStage());
   vStages.push_back(p2dTests.getStage());
   
   mProgressTracker.initialize(pProgress, "Running all tests...", "app", "3E4C8879-97FA-4ddb-B6A5-60BAA437E609");
   mProgressTracker.subdivideCurrentStage(vStages);
   bSuccess = pwarpTests.run(1);
   mProgressTracker.nextStage();
   if (bSuccess)
   {
      bSuccess = p2dTests.run(1);
   }  
   return bSuccess;
}
