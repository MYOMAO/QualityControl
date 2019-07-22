///
/// \file   SimpleDS.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <sstream>

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
  
  // TODO: decide on these histos
  
  for (int j = 0; j < 1; j++) {
    for (int i = 0; i < NStaves[j]; i++) {

      LayHIT[i]->GetZaxis()->SetTitle("Number of Hits");
      LayHIT[i]->GetXaxis()->SetNdivisions(-32);
      LayHIT[i]->Draw("COLZ");
      ConfirmXAxis(LayHIT[i]);
      ReverseYAxis(LayHIT[i]);
      getObjectsManager()->startPublishing(LayHIT[i]);
    }
  }

  for (int j = 0; j < 1; j++) {
    for (int i = 0; i < NChipLay[j]; i++) {
      getObjectsManager()->startPublishing(LayHITNoisy[i]);
    }
  }


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

  getProcessStatus(ctx.inputs().get<int>("Finish"), FileFinish);
  updateFile(ctx.inputs().get<int>("Run"), FileID = ctx.inputs().get<int>("File"));

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
    //InfoCanvas->SetBinContent(3,NEvent);

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

    if (ChipID != ChipIDPre) {
      OccupancyPlot[lay]->Fill(OccupancyCounter);
      OccupancyCounter = 0;
    }
    OccupancyCounter = OccupancyCounter + 1;

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "Fill Occ =  " << difference << "ns" << std::endl;
    }

    int ChipNumber = (ChipID - ChipBoundary[lay]) - sta * NStaveChip[lay];

    // TODO
    LayChipStave[lay]->Fill(ChipNumber, sta);

    int hicCol, hicRow;
    // Todo: check if chipID is really chip ID
    getHicCoordinates(lay, ChipID, col, row, hicCol, hicRow);

    HITMAP[lay][sta][mod]->Fill(hicCol, hicRow);
    chipHitmap[lay][sta][mod][ChipID]->Fill(col, row);

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
    LayEtaPhi[lay]->Fill(eta, phi);

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::nanoseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "After glo etaphi =  " << difference << "ns" << std::endl;
    }

    NEventPre = NEvent;
    ChipIDPre = ChipID;

    if (Counted < TotalCounted) {
      end = std::chrono::high_resolution_clock::now();
      difference = std::chrono::duration_cast < std::chrono::milliseconds > (end - startLoop).count();
      //	QcInfoLogger::GetInstance() << "After Geo = " << difference << "ns" <<  AliceO2::InfoLogger::InfoLogger::endm;
      timefout2 << "End of Vec " << difference << "ns" << std::endl;
      Counted = Counted + 1;
    }

  } // end digits loop

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast < std::chrono::milliseconds > (end - start).count();
  QcInfoLogger::GetInstance() << "Time After Loop = " << difference / 1000.0 << "s"
      << AliceO2::InfoLogger::InfoLogger::endm;
  timefout << "Time After Loop = " << difference / 1000.0 << "s" << std::endl;

  cout << "NEventDone = " << NEvent << endl;
  cout << "START Noisy Pixel Hist" << endl;
  //  if (NEvent > 0 && ChipID > 0 && row > 0 && col > 0) {
  //  for (int j = 0; j < 1; j++) {
  //    for (int i = 0; i < NChipLay[j]; i++) {
  //      LayHITNoisy[i]->Reset();
  //      for (int k = 1; k < NColHis + 1; k++) {
  //        for (int l = 1; l < NRowHis + 1; l++) {
  //          TotalHits = HITMAP[i]->GetBinContent(k, l);
  //          PixelOcc = double(TotalHits) / double(NEvent);
  //          //if(TotalHits > 0) cout << "i = " << i << "   TotalHits = " << TotalHits << "  PixelOcc = " << PixelOcc << endl;
  //          LayHITNoisy[i]->Fill(PixelOcc);
  //        }
  //      }
  //    }
  //  }

  cout << "Done Noisy Pixel Hist" << endl;

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

  ChipStave = new TH2S("ChipStaveCheck", "Stave 1 Layer 1, NHits vs Chip ID", 9, 0, 9, 100, 0, 1500);
  formatAxes(ChipStave, "Chip ID", "Number of Hits", 1., 1.);
  ChipStave->SetMinimum(1);
  
  ErrorFile = new TH2D("ITSQC/General/ErrorFile", "Decoding Errors vs File ID", NFiles + 1, -0.5, NFiles + 0.5, NError, 0.5, NError + 0.5);
  formatAxes(ErrorFile, "File ID (data-link)", "Error ID");
  ErrorFile->GetZaxis()->SetTitle("Counts");
  ErrorFile->SetMinimum(0);

  InfoCanvas = new TH1D("ITSQC/General/InfoCanvas", "InfoCanvas", 3, -0.5, 2.5);
  bulb = new TEllipse(0.2, 0.75, 0.30, 0.20);
}

void SimpleDS::createLayerHistos(int aLayer)
{
  TString Name, Title;

  OccupancyPlot[aLayer] = new TH1D(Form("ITSQC/Occupancy/Layer%dOccupancy", aLayer),
      Form("ITS Layer %d Occupancy Distribution", aLayer), NEventMax[aLayer], 0, NEventMax[aLayer]);
  formatAxes(OccupancyPlot[aLayer], "Occupancy", "Counts", 1., 2.2);

  OccupancyPlotNoisy[aLayer] = new TH1D(Form("ITSQC/Occupancy/Layer%dOccupancyNoisy", aLayer),
      Form("ITS Layer %d, Noisy Pixel Occupancy", aLayer), NEventMax[aLayer], 0, NEventMax[aLayer]);
  formatAxes(OccupancyPlotNoisy[aLayer], "Noisy Pixel Occupancy", "Counts", 1., 2.2);


  LayEtaPhi[aLayer] = new TH2S(Form("ITSQC/Occupancy/Layer%d/Layer%dEtaPhi", aLayer, aLayer),
      Form("ITS Layer%d, Hits vs Eta and Phi", aLayer), NEta, EtaMin, EtaMax, NPhi, PhiMin, PhiMax);
  formatAxes(LayEtaPhi[aLayer], "#eta", "#phi", 1., 1.1);
  LayEtaPhi[aLayer]->GetZaxis()->SetTitle("Number of Hits");
  LayEtaPhi[aLayer]->GetZaxis()->SetTitleOffset(1.4);

  LayChipStave[aLayer] = new TH2S(Form("ITSQC/Occupancy/Layer%d/Layer%dChipStave", aLayer, aLayer),
      Form("ITS Layer%d, Hits vs Chip and Stave", aLayer), NStaveChip[aLayer], 0, NStaveChip[aLayer], NStaves[aLayer], 0, NStaves[aLayer]);
  formatAxes(LayChipStave[aLayer], "Chip Number", "Stave Number", 1., 1.1);
  LayChipStave[aLayer]->GetZaxis()->SetTitle("Number of Hits");
  LayChipStave[aLayer]->GetZaxis()->SetTitleOffset(1.4);

  // HITMAPS per HIC, binning in groups of SizeReduce * SizeReduce pixels
  // chipHitmap: fine binning, one hitmap per chip, but not to be saved to CCDB (only for determination of noisy pixels)
  for (int iStave = 0; iStave < NStaves[aLayer]; iStave ++) {
    createStaveHistos(aLayer, iStave);
  }

  // TODO: decide what to do with this... 
  if (aLayer == 0) {
    for (int iChip = 0; iChip < NChipLay[0]; iChip++) {
      DoubleColOccupancyPlot[aLayer] = new TH1D(
          Form("ITSQC/Occupancy/Layer%d/DoubleCol/Layer%dChip%dDoubleColumnOcc", 0, 0, iChip),
          Form("DCol Occupancy Layer 0, Chip %d", iChip), NColHis / 2, 0, NColHis / 2);
      formatAxes(DoubleColOccupancyPlot[iChip], "Double Column", "Hits", 1.1, 2.2);
    }
  }
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
Title = Form("Hits on Layer %d, Stave %d", aLayer, iStave);
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
  HITMAP[aLayer][aStave][aHic] = new TH2S(Name, Title, nBinsX, 0, maxX, nBinsY, 0,  maxY);
  formatAxes(HITMAP[aLayer][aStave][aHic], "Column", "Row", 1., 1.1);
  // formatting, moved here from initialize
  HITMAP[aLayer][aStave][aHic]->GetZaxis()->SetTitleOffset(1.50);
  HITMAP[aLayer][aStave][aHic]->GetZaxis()->SetTitle("Number of Hits");
  HITMAP[aLayer][aStave][aHic]->GetXaxis()->SetNdivisions(-32);
  HITMAP[aLayer][aStave][aHic]->Draw("COLZ"); // should this really be drawn here?

  for (int iChip = 0; iChip < nChips; iChip ++) {
    chipHitmap[aLayer][aStave][aHic][iChip] = new TH2S(Form("chipHitmapL%dS%dH%dC%d", aLayer, aStave, aHic, iChip),
     Form("chipHitmapL%dS%dH%dC%d", aLayer, aStave, aHic, iChip),  1024, -.5, 1023.5, 512, -.5, 511.5);
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
  getObjectsManager()->startPublishing(FileNameInfo);

  getObjectsManager()->startPublishing(ChipStave);
  getObjectsManager()->startPublishing(ErrorPlots);
  getObjectsManager()->startPublishing(ErrorFile);

  getObjectsManager()->startPublishing(InfoCanvas);

  for (int iLayer = 0; iLayer < NLayer; iLayer ++) {
    if (!layerEnable[iLayer]) continue;
    getObjectsManager()->startPublishing(LayEtaPhi[iLayer]);
    getObjectsManager()->startPublishing(LayChipStave[iLayer]);
    getObjectsManager()->startPublishing(OccupancyPlot[iLayer]);
    getObjectsManager()->startPublishing(OccupancyPlotNoisy[iLayer]);
    for (int iStave = 0; iStave < NStaves[iLayer]; iStave ++) {
      for (int iHic = 0; iHic < nHicPerStave[iLayer]; iHic++) {	 
        getObjectsManager()->startPublishing(HITMAP[iLayer][iStave][iHic]);
      }
    }
  }

  for (int i = 0; i < NChipLay[0]; i++) {
    getObjectsManager()->startPublishing(DoubleColOccupancyPlot[i]);
  }
  
}

void SimpleDS::addMetadata(int runID, int fileID)
{
    getObjectsManager()->addMetadata(ErrorPlots->GetName(), "Run", Form("%d", runID));
    getObjectsManager()->addMetadata(ErrorPlots->GetName(), "File", Form("%d", fileID));

    getObjectsManager()->addMetadata(ErrorFile->GetName(), "Run", Form("%d", runID));
    getObjectsManager()->addMetadata(ErrorFile->GetName(), "File", Form("%d", fileID));

    for (int iLayer = 0; iLayer < NLayer; iLayer ++) {
      if (!layerEnable[iLayer]) continue;
      for (int iStave = 0; iStave < NStaves[iLayer]; iStave ++) {
        for (int iHic = 0; iHic < nHicPerStave[iLayer]; iHic++) {	 
          getObjectsManager()->addMetadata(HITMAP[iLayer][iStave][iHic]->GetName(), "Run", Form("%d", runID));
          getObjectsManager()->addMetadata(HITMAP[iLayer][iStave][iHic]->GetName(), "File", Form("%d", fileID));
	} 
      }
    }

    for (int j = 0; j < 1; j++) {
      for (int i = 0; i < NChipLay[j]; i++) {
        getObjectsManager()->addMetadata(LayHITNoisy[i]->GetName(), "Run", Form("%d", runID));
        getObjectsManager()->addMetadata(LayHITNoisy[i]->GetName(), "File", Form("%d", fileID));

      }
    }

    for (int j = 0; j < 1; j++) {
      for (int i = 0; i < NStaves[j]; i++) {
        getObjectsManager()->addMetadata(LayHIT[i]->GetName(), "Run", Form("%d", runID));
        getObjectsManager()->addMetadata(LayHIT[i]->GetName(), "File", Form("%d", fileID));
      }
    }

    
    for (int i = 0; i < NLayer; i++) {
      getObjectsManager()->addMetadata(LayEtaPhi[i]->GetName(), "Run", Form("%d", runID));
      getObjectsManager()->addMetadata(LayEtaPhi[i]->GetName(), "File", Form("%d", fileID));
      getObjectsManager()->addMetadata(LayChipStave[i]->GetName(), "Run", Form("%d", runID));
      getObjectsManager()->addMetadata(LayChipStave[i]->GetName(), "File", Form("%d", fileID));
      getObjectsManager()->addMetadata(OccupancyPlot[i]->GetName(), "Run", Form("%d", runID));
      getObjectsManager()->addMetadata(OccupancyPlot[i]->GetName(), "File", Form("%d", fileID));
      getObjectsManager()->addMetadata(OccupancyPlotNoisy[i]->GetName(), "Run", Form("%d", runID));
      getObjectsManager()->addMetadata(OccupancyPlotNoisy[i]->GetName(), "File", Form("%d", fileID));

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

void SimpleDS::updateFile(int aRunId, int aFileId)
{

  static int RunIdPre, FileIDPre;
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
  ChipStave->Reset();
  for (int i = 0; i < NLayer; i++) {
    OccupancyPlot[i]->Reset();
    OccupancyPlotNoisy[i]->Reset();
    LayEtaPhi[i]->Reset();
    LayChipStave[i]->Reset();
  }

  for (int j = 0; j < 1; j++) {
    for (int i = 0; i < NStaves[j]; i++) {
      LayHIT[i]->Reset();

    }
  }

  for (int i = 0; i < NChipLay[0]; i++) {
    DoubleColOccupancyPlot[i]->Reset();
  }

  /*
   for(int j = 0; j < 1; j++){
   for(int i = 0; i < NStaveChip[j]; i++){
   HITMAP[i]->Reset();
   }
   }
   */
  for (int iLayer = 0; iLayer < NLayer; iLayer ++) {
    if (!layerEnable[iLayer]) continue;
    for (int iStave = 0; iStave < NStaves[iLayer]; iStave ++) {
      for (int iHic = 0; iHic < nHicPerStave[iLayer]; iHic++) {
        HITMAP[iLayer][iStave][iHic]->Reset();
      }
    }
  }
  
  for (int j = 0; j < 1; j++) {
    for (int i = 0; i < NChipLay[j]; i++) {
      LayHITNoisy[i]->Reset();
    }
  }

  ErrorPlots->Reset();
  NEventInRun = 0;
  ErrorFile->Reset();
  TotalFileDone = 0;
  ptNFile->Clear();
  ptNFile->AddText(Form("File Processed: %d ", TotalFileDone));
  Yellowed = 0;
  for (int j = 0; j < 1; j++) {
    for (int i = 0; i < NStaves[j]; i++) {
      LayHIT[i]->GetXaxis()->SetNdivisions(-32);
      ConfirmXAxis(LayHIT[i]);
      ReverseYAxis(LayHIT[i]);
    }
  }

  QcInfoLogger::GetInstance() << "DONE the histogram Resetting" << AliceO2::InfoLogger::InfoLogger::endm;

}

} // namespace simpleds
} // namespace quality_control_modules
} // namespace o2

