///
/// \file   SimpleDS.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <sstream>
#include <math.h>
#include <TStopwatch.h>
#include "DataFormatsParameters/GRPObject.h"
#include "FairLogger.h"
#include "FairRunAna.h"
#include "FairFileSource.h"
#include "FairRuntimeDb.h"
#include "FairParRootFileIo.h"
#include "FairSystemInfo.h"

#include <TCanvas.h>

#include "QualityControl/FileFinish.h"
#include "QualityControl/QcInfoLogger.h"
#include "SimpleDS/SimpleDS.h"
#include "DetectorsBase/GeometryManager.h"
#include "ITSBase/GeometryTGeo.h"
#include "ITSMFTReconstruction/DigitPixelReader.h"
#include <algorithm>

using o2::itsmft::Digit;

using namespace std;
using namespace o2::itsmft;
using namespace o2::its;

namespace o2 {
namespace quality_control_modules {
namespace simpleds {

SimpleDS::SimpleDS()
    :
    TaskInterface()
{

  o2::base::GeometryManager::loadGeometry();
  gStyle->SetPadRightMargin(0.15);
  gStyle->SetPadLeftMargin(0.15);

  gStyle->SetOptFit(0);
  gStyle->SetOptStat(0);

  for (int i = 0; i < NLayer; i++) {
    NChipLay[i] = ChipBoundary[i + 1] - ChipBoundary[i];
    NStaveChip[i] = NChipLay[i] / NStaves[i];
    NColStave[i] = NStaveChip[i] * NColHis;
  }
  
  m_objects.clear();
  m_publishedObjects.clear();
  createHistos();

  for (int i = 0; i < NError; i++) {
    Errors[i] = 0;
    ErrorPre[i] = 0;
    ErrorPerFile[i] = 0;
  }
}

SimpleDS::~SimpleDS()
{

}

void SimpleDS::initialize(o2::framework::InitContext &ctx)
{
  QcInfoLogger::GetInstance() << "initialize SimpleDS" << AliceO2::InfoLogger::InfoLogger::endm;

  o2::its::GeometryTGeo *geom = o2::its::GeometryTGeo::Instance();
  geom->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
  numOfChips = geom->getNumberOfChips();
  cout << "numOfChips = " << numOfChips << endl;
  setNChips(numOfChips);

  for (int i = 0; i < NError; i++) {
    pt[i] = new TPaveText(0.20, 0.80 - i * 0.05, 0.85, 0.85 - i * 0.05, "NDC");
    formatPaveText(pt[i], 0.04, gStyle->GetTextColor(), 12, ErrorType[i].Data());
    ErrorPlots->GetListOfFunctions()->Add(pt[i]);
  }

  ptFileName = new TPaveText(0.20, 0.40, 0.85, 0.50, "NDC");
  formatPaveText(ptFileName, 0.04, gStyle->GetTextColor(), 12, "Current File Processing: ");

  ptNFile = new TPaveText(0.20, 0.30, 0.85, 0.40, "NDC");
  formatPaveText(ptNFile, 0.04, gStyle->GetTextColor(), 12, "File Processed: ");  

  ptNEvent = new TPaveText(0.20, 0.20, 0.85, 0.30, "NDC");
  formatPaveText(ptNEvent, 0.04, gStyle->GetTextColor(), 12, "Event Processed: ");    

  bulbRed = new TPaveText(0.60, 0.75, 0.90, 0.85, "NDC");
  formatPaveText(bulbRed, 0.04, kRed, 12, "Red = QC Waiting");      

  bulbYellow = new TPaveText(0.60, 0.65, 0.90, 0.75, "NDC");
  formatPaveText(bulbYellow, 0.04, kYellow, 12, "Yellow = QC Pausing");        

  bulbGreen = new TPaveText(0.60, 0.55, 0.90, 0.65, "NDC");
  formatPaveText(bulbGreen, 0.04, kGreen, 12, "Green= QC Processing");

  InfoCanvas->SetTitle("QC Process Information Canvas");
  InfoCanvas->GetListOfFunctions()->Add(ptFileName);
  InfoCanvas->GetListOfFunctions()->Add(ptNFile);
  InfoCanvas->GetListOfFunctions()->Add(ptNEvent);
  InfoCanvas->GetListOfFunctions()->Add(bulb);
  InfoCanvas->GetListOfFunctions()->Add(bulbRed);
  InfoCanvas->GetListOfFunctions()->Add(bulbYellow);
  InfoCanvas->GetListOfFunctions()->Add(bulbGreen);
  //		InfoCanvas->SetStats(false);

  publishHistos();
  
  cout << "DONE Inititing Publication = " << endl;

  bulb->SetFillColor(kRed);
  TotalFileDone = 0;
  TotalHisTime = 0;
  Counted = 0;
  Yellowed = 0;
}

void SimpleDS::startOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

}

void SimpleDS::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SimpleDS::monitorData(o2::framework::ProcessingContext &ctx)
{

  start = std::chrono::high_resolution_clock::now();

  ofstream timefout("HisTimeGlobal.dat", ios::app);

  ofstream timefout2("HisTimeLoop.dat", ios::app);

  QcInfoLogger::GetInstance() << "BEEN HERE BRO" << AliceO2::InfoLogger::InfoLogger::endm;

  int FileID = ctx.inputs().get<int>("File");
  getProcessStatus(ctx.inputs().get<int>("Finish"), FileFinish);
  updateFile(ctx.inputs().get<int>("Run"), FileID); 

  //Will Fix Later//

  int ResetDecision = ctx.inputs().get<int>("in");
  QcInfoLogger::GetInstance() << "Reset Histogram Decision = " << ResetDecision
      << AliceO2::InfoLogger::InfoLogger::endm;
  if (ResetDecision == 1)
    reset();

  auto digits = ctx.inputs().get<const std::vector<o2::itsmft::Digit>>("digits");
  LOG(INFO) << "Digit Size Getting For This TimeFrame (Event) = " << digits.size();

  Errors = ctx.inputs().get<const std::array<unsigned int, NError>>("Error");

  for (int i = 0; i < NError; i++) {
    ErrorPerFile[i] = Errors[i] - ErrorPre[i];
  }

  for (int i = 0; i < NError; i++) {
    QcInfoLogger::GetInstance() << " i = " << i << "   Error = " << Errors[i] << "   ErrorPre = " << ErrorPre[i]
        << "   ErrorPerFile = " << ErrorPerFile[i] << AliceO2::InfoLogger::InfoLogger::endm;
    ErrorPlots->SetBinContent(i + 1, Errors[i]);
    ErrorFile->SetBinContent(FileID + 1, i + 1, ErrorPerFile[i]);
  }

  if (FileFinish == 1) {
    for (int i = 0; i < NError; i++) {
      ErrorPre[i] = Errors[i];
    }
  }

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast < std::chrono::milliseconds > (end - start).count();
  //	QcInfoLogger::GetInstance() << "Before Loop = " << difference/1000.0 << "s" <<  AliceO2::InfoLogger::InfoLogger::endm;
  timefout << "Before Loop  = " << difference / 1000.0 << "s" << std::endl;

  for (auto &&pixeldata : digits) {
    startLoop = std::chrono::high_resolution_clock::now();

    ChipID = pixeldata.getChipIndex();
    col = pixeldata.getColumn();
    row = pixeldata.getRow();
    NEvent = pixeldata.getROFrame();

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "Before Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "Getting Value Time  = " << difference << "ns" << std::endl;
    }

    if (NEvent % 1000000 == 0 && NEvent > 0)
      cout << "ChipID = " << ChipID << "  col = " << col << "  row = " << row << "  NEvent = " << NEvent << endl;

    // wouldnt this update this update the text for every digit in events 1000, 2000 ... ?
    if (NEvent % 1000 == 0 || NEventPre != NEvent) {
      ptNEvent->Clear();
      ptNEvent->AddText(Form("Event Being Processed: %d", NEvent));
    }

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "Before Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "Before Geo  = " << difference << "ns" << std::endl;
    }

    gm->getChipId(ChipID, lay, sta, ssta, mod, chip);
    gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
    const Point3D<float> loc(0., 0., 0.);
    auto glo = gm->getMatrixL2G(ChipID)(loc);

    if (!layerEnable[lay]) continue;
    
    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "After Geo =  " << difference << "ns" << std::endl;
    }

    int hicCol, hicRow;
    // Todo: check if chipID is really chip ID
    getHicCoordinates(lay, ChipID, col, row, hicCol, hicRow);

    hHITMAP[lay][sta][mod]->Fill(hicCol, hicRow);
    hChipHitmap[lay][sta][mod][chip]->Fill(col, row);

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "Fill HitMaps =  " << difference << "ns" << std::endl;
    }

    // if (lay == 0)
    //   DoubleColOccupancyPlot[ChipID]->Fill(col / 2);

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "Before glo etaphi =  " << difference << "ns" << std::endl;
    }

    eta = glo.eta();
    phi = glo.phi();
    hEtaPhiHitmap[lay]->Fill(eta, phi);

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "After glo etaphi =  " << difference << "ns" << std::endl;
      Counted = Counted + 1;
    }

    NEventPre = NEvent;

  } // end digits loop

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast < std::chrono::milliseconds > (end - start).count();
  QcInfoLogger::GetInstance() << "Time After Loop = " << difference / 1000.0 << "s"
      << AliceO2::InfoLogger::InfoLogger::endm;
  timefout << "Time After Loop = " << difference / 1000.0 << "s" << std::endl;

  cout << "NEventDone = " << NEvent << endl;
  cout <<  "Test  " << endl;

  digits.clear();

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast < std::chrono::milliseconds > (end - start).count();
  TotalHisTime = TotalHisTime + difference;
  QcInfoLogger::GetInstance() << "Time in Histogram = " << difference / 1000.0 << "s"
      << AliceO2::InfoLogger::InfoLogger::endm;
  timefout << "Time in Histogram = " << difference / 1000.0 << "s" << std::endl;

  if (NEvent == 0 && ChipID == 0 && row == 0 && col == 0 && Yellowed == 0) {
    bulb->SetFillColor(kYellow);
    Yellowed = 1;
  }

}

void SimpleDS::addObject(TObject* aObject, bool published)
{
  if (!aObject) {
    std::cout << "ERROR: trying to add non-existent object" << std::endl;
    return;
  }
  m_objects.push_back(aObject);
  if (published) {
    m_publishedObjects.push_back(aObject);
  }
}

void SimpleDS::createHistos()
{
  createGlobalHistos();
  
  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!layerEnable[iLayer]) continue;
    createLayerHistos(iLayer);
  }
}

void SimpleDS::createGlobalHistos()
{
  ErrorPlots = new TH1D("ITSQC/General/ErrorPlots", "Decoding Errors", NError, 0.5, NError + 0.5);
  formatAxes(ErrorPlots, "Error ID", "Counts");
  ErrorPlots->SetMinimum(0);
  ErrorPlots->SetFillColor(kRed);

  FileNameInfo = new TH1D("ITSQC/General/FileNameInfo", "FileNameInfo", 5, 0, 1);  
  formatAxes(FileNameInfo, "InputFile", "Total Files Processed", 1.1);

  ErrorFile = new TH2D("ITSQC/General/ErrorFile", "Decoding Errors vs File ID", NFiles + 1, -0.5, NFiles + 0.5, NError, 0.5, NError + 0.5);
  formatAxes(ErrorFile, "File ID (data-link)", "Error ID");
  ErrorFile->GetZaxis()->SetTitle("Counts");
  ErrorFile->SetMinimum(0);

  InfoCanvas = new TH1D("ITSQC/General/InfoCanvas", "InfoCanvas", 3, -0.5, 2.5);
  bulb = new TEllipse(0.2, 0.75, 0.30, 0.20);

  addObject(ErrorPlots);
  addObject(ErrorFile);
}

void SimpleDS::createLayerHistos(int aLayer)
{
  createEtaPhiHitmap(aLayer);
  createChipStaveOcc(aLayer);
  
  // 1d- occupancy histogram of the full layer, x-axis units = log (occupancy)
  hOccupancyPlot[aLayer] = new TH1D(Form("ITSQC/Occupancy/Layer%dOccupancy", aLayer),
				    Form("ITS Layer %d Occupancy Distribution", aLayer), 300, -15, 0);
  formatAxes(hOccupancyPlot[aLayer], "Occupancy", "Counts", 1., 2.2);
  addObject(hOccupancyPlot[aLayer]);

  // HITMAPS per HIC, binning in groups of SizeReduce * SizeReduce pixels
  // chipHitmap: fine binning, one hitmap per chip, but not to be saved to CCDB (only for determination of noisy pixels)
  for (int iStave = 0; iStave < NStaves[aLayer]; iStave ++) {
    createStaveHistos(aLayer, iStave);
  }

  // TODO: decide what to do with this... 
  //  if (aLayer == 0) {
  //  for (int iChip = 0; iChip < NChipLay[0]; iChip++) {
  //    DoubleColOccupancyPlot[aLayer] = new TH1D(
  //        Form("ITSQC/Occupancy/Layer%d/DoubleCol/Layer%dChip%dDoubleColumnOcc", 0, 0, iChip),
  //        Form("DCol Occupancy Layer 0, Chip %d", iChip), NColHis / 2, 0, NColHis / 2);
  //    formatAxes(DoubleColOccupancyPlot[aLayer], "Double Column", "Hits", 1.1, 2.2);
  //  }
    //}
}

// hChipStaveOccupancy: Occupancy histograms for complete layer
// y-axis: number of stave
// x-axis: number of chip (IB) or number of HIC (OB)
void SimpleDS::createChipStaveOcc(int aLayer)
{
  int nBinsX;
  if (aLayer < NLayerIB) {
    nBinsX = nChipsPerHic[aLayer];
    hChipStaveOccupancy[aLayer] = new TH2S(Form("ITSQC/Occupancy/Layer%d/Layer%dChipStave", aLayer, aLayer),
        Form("ITS Layer%d, Hits vs Chip and Stave", aLayer), nBinsX, -.5 , nBinsX-.5 , NStaves[aLayer], -.5, NStaves[aLayer]-.5);
    formatAxes(hChipStaveOccupancy[aLayer], "Chip Number", "Stave Number", 1., 1.1);
  }
  else {
    nBinsX = nHicPerStave[aLayer];
    hChipStaveOccupancy[aLayer] = new TH2S(Form("ITSQC/Occupancy/Layer%d/Layer%dHicStave", aLayer, aLayer),
        Form("ITS Layer%d, Hits vs Hic and Stave", aLayer), nBinsX, -.5 , nBinsX-.5 , NStaves[aLayer], -.5, NStaves[aLayer]-.5);
    formatAxes(hChipStaveOccupancy[aLayer], "Hic Number", "Stave Number", 1., 1.1);
  }
  
  hChipStaveOccupancy[aLayer]->GetZaxis()->SetTitle("Number of Hits");
  hChipStaveOccupancy[aLayer]->GetZaxis()->SetTitleOffset(1.4);
  addObject(hChipStaveOccupancy[aLayer]);
}

// hEtaPhiHitmap: eta-phi hitmaps for complete layers, binning 100 x 100 pixels
// using eta coverage of TDR, assuming phi runs from 0 ... 2*Pi
void SimpleDS::createEtaPhiHitmap(int aLayer)
{
  int NEta, NPhi;
  if (aLayer < NLayerIB) {
    NEta = 9 * 10;
    NPhi = NStaves[aLayer] * 5;
  }
  else {
    NEta = nHicPerStave[aLayer] * 70;
    NPhi = NStaves[aLayer] * 10;    
  }
  hEtaPhiHitmap[aLayer] = new TH2S(Form("ITSQC/Occupancy/Layer%d/Layer%dEtaPhi", aLayer, aLayer),
      Form("ITS Layer%d, Hits vs Eta and Phi", aLayer), NEta, (-1)*etaCoverage[aLayer], etaCoverage[aLayer], NPhi, PhiMin, PhiMax);
  formatAxes(hEtaPhiHitmap[aLayer], "#eta", "#phi", 1., 1.1);
  hEtaPhiHitmap[aLayer]->GetZaxis()->SetTitle("Number of Hits");
  hEtaPhiHitmap[aLayer]->GetZaxis()->SetTitleOffset(1.4);
  addObject(hEtaPhiHitmap[aLayer]);
}

void SimpleDS::createStaveHistos(int aLayer, int aStave)
{
  for (int iHic = 0; iHic < nHicPerStave[aLayer]; iHic++) {
     createHicHistos(aLayer, aStave, iHic);
  }
}


void SimpleDS::createHicHistos(int aLayer, int aStave, int aHic)
{
  TString Name, Title;
  int nBinsX, nBinsY, maxX, maxY, nChips;

  if (aLayer < NLayerIB) {
    Name = Form("ITSQC/Occupancy/Layer%d/Stave%d/Layer%dStave%dHITMAP", aLayer, aStave, aLayer, aStave);
    Title = Form("Hits on Layer %d, Stave %d", aLayer, aStave);
    maxX = 9 * NColHis;
    maxY = NRowHis;
nChips = 9;
  }
  else {
    Name = Form("ITSQC/Occupancy/Layer%d/Stave%d/HIC%d/Layer%dStave%dHIC%dHITMAP", aLayer, aStave, aHic, aLayer, aStave, aHic);
Title = Form("Hits on Layer %d, Stave %d, Hic %d", aLayer, aStave, aHic);
    maxX = 7 * NColHis;
    maxY = 2 * NRowHis;
    nChips = 14;
  }
  nBinsX = maxX / SizeReduce;
  nBinsY = maxY / SizeReduce;
  hHITMAP[aLayer][aStave][aHic] = new TH2S(Name, Title, nBinsX, 0, maxX, nBinsY, 0,  maxY);
  formatAxes(hHITMAP[aLayer][aStave][aHic], "Column", "Row", 1., 1.1);
  // formatting, moved here from initialize
  hHITMAP[aLayer][aStave][aHic]->GetZaxis()->SetTitleOffset(1.50);
  hHITMAP[aLayer][aStave][aHic]->GetZaxis()->SetTitle("Number of Hits");
  hHITMAP[aLayer][aStave][aHic]->GetXaxis()->SetNdivisions(-32);
  hHITMAP[aLayer][aStave][aHic]->Draw("COLZ"); // should this really be drawn here?
  addObject(hHITMAP[aLayer][aStave][aHic]);

  for (int iChip = 0; iChip < nChips; iChip ++) {
    hChipHitmap[aLayer][aStave][aHic][iChip] = new TH2S(Form("chipHitmapL%dS%dH%dC%d", aLayer, aStave, aHic, iChip),
    Form("chipHitmapL%dS%dH%dC%d", aLayer, aStave, aHic, iChip),  1024, -.5, 1023.5, 512, -.5, 511.5);
    addObject(hChipHitmap[aLayer][aStave][aHic][iChip], false);
  }
}

// To be checked: 
// - something like this should exist in the official geometry already
// - is aChip really the chipID (i.e. 0..6, 8.. 14 in case of OB HICs)?
void SimpleDS::getHicCoordinates (int aLayer, int aChip, int aCol, int aRow, int& aHicRow, int& aHicCol)
{
  aChip &= 0xf;
  if (aLayer < NLayerIB) {
    aHicCol = aChip * NCols + aCol;
    aHicRow = aRow;
  }
  else { // OB Hic: chip row 0 at center of HIC
    if (aChip < 7) {
      aHicCol = aChip * NCols + aCol;
      aHicRow = NRows - aRow - 1;      
    }
    else {
      aHicRow = NRows + aRow; 
      aHicCol = 7 * NCols - ((aChip - 8) * NCols + aCol);
    }
  }
}

void SimpleDS::formatAxes(TH1 *h, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
{
  h->GetXaxis()->SetTitle(xTitle);
  h->GetYaxis()->SetTitle(yTitle);
  h->GetXaxis()->SetTitleOffset(xOffset);
  h->GetYaxis()->SetTitleOffset(yOffset);
}

void SimpleDS::formatPaveText(TPaveText *aPT, float aTextSize, Color_t aTextColor, short aTextAlign, const char *aText)
{
  aPT->SetTextSize(aTextSize);
  aPT->SetTextAlign(aTextAlign);
  aPT->SetFillColor(0);
  aPT->SetTextColor(aTextColor);
  aPT->AddText(aText);  
}

void SimpleDS::ConfirmXAxis(TH1 *h)
{
  // Remove the current axis
  h->GetXaxis()->SetLabelOffset(999);
  h->GetXaxis()->SetTickLength(0);
  // Redraw the new axis
  gPad->Update();
  XTicks = (h->GetXaxis()->GetXmax() - h->GetXaxis()->GetXmin()) / DivisionStep;

  TGaxis *newaxis = new TGaxis(gPad->GetUxmin(), gPad->GetUymin(), gPad->GetUxmax(), gPad->GetUymin(),
      h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax(), XTicks, "N");
  newaxis->SetLabelOffset(0.0);
  newaxis->Draw();
  h->GetListOfFunctions()->Add(newaxis);
}

void SimpleDS::ReverseYAxis(TH1 *h)
{
  // Remove the current axis
  h->GetYaxis()->SetLabelOffset(999);
  h->GetYaxis()->SetTickLength(0);

  // Redraw the new axis
  gPad->Update();

  YTicks = (h->GetYaxis()->GetXmax() - h->GetYaxis()->GetXmin()) / DivisionStep;
  TGaxis *newaxis = new TGaxis(gPad->GetUxmin(), gPad->GetUymax(), gPad->GetUxmin() - 0.001, gPad->GetUymin(),
      h->GetYaxis()->GetXmin(), h->GetYaxis()->GetXmax(), YTicks, "N");

  newaxis->SetLabelOffset(0);
  newaxis->Draw();
  h->GetListOfFunctions()->Add(newaxis);

}

void SimpleDS::publishHistos()
{
  for (unsigned int iObj = 0; iObj < m_publishedObjects.size(); iObj++) {
    getObjectsManager()->startPublishing(m_publishedObjects.at(iObj));
  }
}

void SimpleDS::addMetadata(int runID, int fileID)
{
  for (unsigned int iObj = 0; iObj < m_publishedObjects.size(); iObj++) {
    getObjectsManager()->addMetadata(m_publishedObjects.at(iObj)->GetName(), "Run", Form("%d", runID));
    getObjectsManager()->addMetadata(m_publishedObjects.at(iObj)->GetName(), "File", Form("%d", fileID));
  }
}

void SimpleDS::getProcessStatus(int aInfoFile, int& aFileFinish)
{
  aFileFinish = aInfoFile % 10;
  int FileRest = (aInfoFile - aFileFinish) / 10;

  QcInfoLogger::GetInstance() << "FileFinish = " << aFileFinish << AliceO2::InfoLogger::InfoLogger::endm;
  QcInfoLogger::GetInstance() << "FileRest = " << FileRest << AliceO2::InfoLogger::InfoLogger::endm;

  if (aFileFinish == 0)
    bulb->SetFillColor(kGreen);
  if (aFileFinish == 1 && FileRest > 1)
    bulb->SetFillColor(kYellow);
  if (aFileFinish == 1 && FileRest == 1)
    bulb->SetFillColor(kRed);
}

void SimpleDS::updateFile(int aRunID, int aFileID)
{

  static int RunIDPre, FileIDPre;
  if (RunIDPre != aRunID || FileIDPre != aFileID) {
    TString FileName = Form("infiles/run000%d/data-link%d", aRunID, aFileID);
    QcInfoLogger::GetInstance() << "For the Moment: RunID = " << aRunID << "  FileID = " << aFileID
        << AliceO2::InfoLogger::InfoLogger::endm;
    FileNameInfo->Fill(0.5);
    FileNameInfo->SetTitle(Form("Current File Name: %s", FileName.Data()));
    TotalFileDone = TotalFileDone + 1;
    //InfoCanvas->SetBinContent(1,FileID);
    //InfoCanvas->SetBinContent(2,TotalFileDone);
    ptFileName->Clear();
    ptNFile->Clear();
    ptFileName->AddText(Form("File Being Proccessed: %s", FileName.Data()));
    ptNFile->AddText(Form("File Processed: %d ", TotalFileDone));

    addMetadata(aRunID, aFileID);
  }
  RunIDPre = aRunID;
  FileIDPre = aFileID;
}

void SimpleDS::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

}

void SimpleDS::endOfActivity(Activity &activity)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void SimpleDS::reset()
{
  // clean all the monitor objects here
  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;

  NEventInRun = 0;
  TotalFileDone = 0;
  ptNFile->Clear();
  ptNFile->AddText(Form("File Processed: %d ", TotalFileDone));
  Yellowed = 0;

  resetHitmaps();
  resetOccupancyPlots();

  QcInfoLogger::GetInstance() << "DONE the histogram Resetting" << AliceO2::InfoLogger::InfoLogger::endm;
}

// reset method for all plots that are supposed to be reset once
void SimpleDS::resetHitmaps()
{
  ErrorPlots->Reset();
  ErrorFile->Reset();
  for (int iLayer = 0; iLayer < NLayer; iLayer ++) {
    if (!layerEnable[iLayer]) continue;
    hEtaPhiHitmap[iLayer]->Reset();
    for (int iStave = 0; iStave < NStaves[iLayer]; iStave ++) {
      for (int iHic = 0; iHic < nHicPerStave[iLayer]; iHic++) {
        hHITMAP[iLayer][iStave][iHic]->Reset();
        for (int iChip = 0; iChip < nChipsPerHic[iLayer]; iChip ++) {
          hChipHitmap[iLayer][iStave][iHic][iChip]->Reset();
        }
      }
    }
  }
  //for (int i = 0; i < NChipLay[0]; i++) {
  //  DoubleColOccupancyPlot[i]->Reset();
  //}

}

// reset method for all histos that are to be reset regularly
// (occupancy plots when recalculating / updating the occupancies)
void SimpleDS::resetOccupancyPlots()
{
  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!layerEnable[iLayer]) continue;
    hOccupancyPlot[iLayer]->Reset();
    hChipStaveOccupancy[iLayer]->Reset();
  }
}

void SimpleDS::updateOccupancyPlots(int nEvents)
{
  double pixelOccupancy, chipOccupancy;
  int nPixels = 512 * 1024;

  resetOccupancyPlots();

  for (int iLayer = 0; iLayer < NLayer; iLayer ++) {
    if (!layerEnable[iLayer]) continue;
    hEtaPhiHitmap[iLayer]->Reset();
    for (int iStave = 0; iStave < NStaves[iLayer]; iStave ++) {
      for (int iHic = 0; iHic < nHicPerStave[iLayer]; iHic++) {
        for (int iChip = 0; iChip < nChipsPerHic[iLayer]; iChip ++) {
          chipOccupancy = hChipHitmap[iLayer][iStave][iHic][iChip]->Integral();
          chipOccupancy /= (nEvents * nPixels);
          if (iLayer < NLayerIB) {
            hChipStaveOccupancy[iLayer]->Fill(iChip, iStave, chipOccupancy);
          }
          else {
            hChipStaveOccupancy[iLayer]->Fill(iHic, iStave, chipOccupancy / nChipsPerHic[iLayer]);
          }
          for (int iCol = 0; iCol < 1024; iCol++){
            for (int iRow = 0; iRow < 512; iRow ++) {
              pixelOccupancy = hChipHitmap[iLayer][iStave][iHic][iChip]->GetBinContent(iCol + 1, iRow + 1);
              if (pixelOccupancy > 0) {
                pixelOccupancy /= nEvents;
                hOccupancyPlot[iLayer]->Fill(log10(pixelOccupancy));
              }
            }
          }
        }
      }
    }
  }
}


} // namespace simpleds
} // namespace quality_control_modules
} // namespace o2

