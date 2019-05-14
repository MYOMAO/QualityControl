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
#include <TH1.h>
#include <TH2.h>


#include "QualityControl/QcInfoLogger.h"
#include "SimpleDS/SimpleDS.h"
#include "DetectorsBase/GeometryManager.h"
#include "ITSBase/GeometryTGeo.h"
#include "ITSMFTReconstruction/DigitPixelReader.h"


using o2::ITSMFT::Digit;



using namespace std;
using namespace o2::ITSMFT;
using namespace o2::ITS;



namespace o2
{
	namespace quality_control_modules
	{
		namespace simpleds
		{

			SimpleDS::SimpleDS() : TaskInterface(), mHistogram(nullptr) { 
				mHistogram = nullptr;

				gStyle->SetPadRightMargin(0.15);
				gStyle->SetPadLeftMargin(0.15);
				o2::base::GeometryManager::loadGeometry ();


				gStyle->SetOptFit(0);
				gStyle->SetOptStat(0);

				ChipStave->GetXaxis()->SetTitle("Chip ID");
				ChipStave->GetYaxis()->SetTitle("Number of Hits");
				ChipStave->SetTitle("Number of Hits vs Chip ID for Stave 1 at Layer 1");


				for(int i = 0; i < NLayer; i++){

					NChipLay[i] = ChipBoundary[i + 1] - ChipBoundary[i];

					OccupancyPlot[i]	= new TH1D(Form("Occupancy%d",i),Form("Occupancy%d",i),NEventMax[i],0,NEventMax[i]); 
					OccupancyPlot[i]->GetXaxis()->SetTitle ("Occupancy");
					OccupancyPlot[i]->GetYaxis()->SetTitle ("Counts");
					OccupancyPlot[i]->GetYaxis()->SetTitleOffset(2.2);	
					OccupancyPlot[i]->SetTitle(Form("Occupancy Distribution for ITS Layer %d",i));

					LayEtaPhi[i] = new TH2D(Form("Lay1EtaPhiLay%d",i),Form("Lay1EtaPhiLay%d",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhi[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhi[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhi[i]->GetZaxis()->SetTitle("Number of Hits");
					LayEtaPhi[i]->GetZaxis()->SetTitleOffset(1.4);
					LayEtaPhi[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayEtaPhi[i]->SetTitle(Form("Number of Hits for Layer %d #eta and #phi Distribution",i));


					NStaveChip[i] = NChipLay[i]/NStaves[i];
					NColStave[i] = NStaveChip[i] * NColHis;

					LayChipStave[i] = new TH2D(Form("LayChipStave%d",i),Form("LayChipStave%d",i),NStaveChip[i],0,NStaveChip[i],NStaves[i],0,NStaves[i]);
					LayChipStave[i]->GetXaxis()->SetTitle("Chip Number");
					LayChipStave[i]->GetYaxis()->SetTitle("Stave Number");
					LayChipStave[i]->GetZaxis()->SetTitle("Number of Hits");
					LayChipStave[i]->GetZaxis()->SetTitleOffset(1.4);
					LayChipStave[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayChipStave[i]->SetTitle(Form("Number of Hits for Layer %d Chip Number and Stave Number Distribution",i));


				}


				for(int i = 0; i < NError; i++){
					Error[i] = 0;
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i< NStaves[j]; i++){
						Lay1HIG[i] = new TH2D(Form("HICMAPLay%dStave%d",j,i),Form("HICMAPLay%dStave%d",j,i),NColHis*NStaveChip[j],0,NColHis*NStaveChip[j],NRowHis,0,NRowHis);
						//		Lay1HIG[i] = new TH2D(Form("HICMAPLay%dStave%d",j,i),Form("HICMAPLay%dStave%d",j,i),100,0,NColHis*NStaveChip[j],100,0,NRowHis);
						Lay1HIG[i]->GetXaxis()->SetTitle("Column");
						Lay1HIG[i]->GetYaxis()->SetTitle("Row");
						Lay1HIG[i]->GetYaxis()->SetTitleOffset(1.10);
						Lay1HIG[i]->GetZaxis()->SetTitleOffset(1.50);
						Lay1HIG[i]->SetTitle(Form("Hits Map on Layer %d Stave %d",j,i));
					}
				}

				ErrorPlots->GetXaxis()->SetTitle("Error ID");
				ErrorPlots->GetYaxis()->SetTitle("Counts");
				ErrorPlots->SetTitle("Error Checked During Decoding");
				ErrorPlots->SetMinimum(0);
				cout << "DONE 1" << endl;
				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HIGMAP[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),NColHis,0,NColHis,NRowHis,0,NRowHis);
						//		HIGMAP[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis,100,0,NRowHis);
						HIGMAP[i]->GetXaxis()->SetTitle("Column");
						HIGMAP[i]->GetYaxis()->SetTitle("Row");
						HIGMAP[i]->GetYaxis()->SetTitleOffset(1.10);
						HIGMAP[i]->GetZaxis()->SetTitleOffset(1.50);
						HIGMAP[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Number % d on Layer %d",i,j));
					}
				}
				cout << "DONE 2" << endl;

				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HIGMAP6[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),NColHis*11,0,NColHis*11,NRowHis,0,NRowHis);
						//		HIGMAP6[i]	= new TH2D(Form("HIGMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis*11,100,0,NRowHis);
						HIGMAP6[i]->GetXaxis()->SetTitle("Column");
						HIGMAP6[i]->GetYaxis()->SetTitle("Row");
						HIGMAP6[i]->GetYaxis()->SetTitleOffset(1.10);
						HIGMAP6[i]->GetZaxis()->SetTitleOffset(1.50);
						HIGMAP6[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Sector Number % d on Layer %d",i,j));
					}
				}
				cout << "DONE 3" << endl;

				for(int i = 0; i < ChipBoundary[NLayer]; i++){
					Occupancy[i] = 0;
				}

				cout << "Clear " << endl;





			}

			SimpleDS::~SimpleDS() {
				if (mHistogram) {
					delete mHistogram;
				}
			}

			void SimpleDS::initialize(o2::framework::InitContext& ctx)
			{
				QcInfoLogger::GetInstance() << "initialize SimpleDS" << AliceO2::InfoLogger::InfoLogger::endm;

				mHistogram = new TH1F("example", "example", 20, 0, 30000);
				getObjectsManager()->startPublishing(mHistogram);
				//getObjectsManager()->addCheck(mHistogram, "checkFromSimpleDS", "o2::quality_control_modules::simpleds::SimpleDSCheck","QcSimpleDS");


			}

			void SimpleDS::startOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

			void SimpleDS::startOfCycle()
			{
				QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void SimpleDS::monitorData(o2::framework::ProcessingContext& ctx)
			{
				// In this function you can access data inputs specified in the JSON config file, for example:
				//  {
				//    "binding": "random",
				//    "dataOrigin": "ITS",
				//    "dataDescription": "RAWDATA"
				//  }

				// Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
				// One can find additional examples at:
				// https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

				// Some examples:
				QcInfoLogger::GetInstance() << "BEEN HERE BRO" << AliceO2::InfoLogger::InfoLogger::endm;
				// 1. In a loop
				QcInfoLogger::GetInstance() << "Now We Start Looping the INFILE" << AliceO2::InfoLogger::InfoLogger::endm;
				
				/*
				int ResetDecision = ctx.inputs().get<int>("in");
				QcInfoLogger::GetInstance() << "Reset Histogram Decision = " << ResetDecision << AliceO2::InfoLogger::InfoLogger::endm;



				if(ResetDecision == 1) reset();
				*/



				//QcInfoLogger::GetInstance() << "Digit Size Got = " << digit.size() << AliceO2::InfoLogger::InfoLogger::endm;

				//auto digits = ctx.inputs().get<const std::vector<o2::ITSMFT::Digit>>("digits");
				//LOG(INFO) << "Digit Size Got = " << digits.size() << " digits";

				/*
				   for (auto&& input : ctx.inputs()) {
				   o2::ITSMFT::Digit digit = ctx.inputs().get<o2::ITSMFT::Digit>("digits");
				   LOG(INFO) << "Chip ID Getting " << digit.getChipIndex() << " Row = " << digit.getRow() << "   Column = " << digit.getColumn();
				   }

				   QcInfoLogger::GetInstance() << "DONE" << AliceO2::InfoLogger::InfoLogger::endm;
				   */

					o2::ITSMFT::Digit digit = ctx.inputs().get<o2::ITSMFT::Digit>("digits");
					LOG(INFO) << "Chip ID Getting " << digit.getChipIndex() << " Row = " << digit.getRow() << "   Column = " << digit.getColumn();


				for (auto&& input : ctx.inputs()) {




				//	o2::ITSMFT::Digit digit = ctx.inputs().get<o2::ITSMFT::Digit>("digits");
					//	LOG(INFO) << "Chip ID Getting " << digit.getChipIndex() << " Row = " << digit.getRow() << "   Column = " << digit.getColumn();
					ChipID = digit.getChipIndex();
					col = digit.getColumn();
					row = digit.getRow();

					gm->getChipId (ChipID, lay, sta, ssta, mod, chip);
					gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
					Occupancy[ChipID] = Occupancy[ChipID] + 1;
					LOG(INFO) << "ChipID = " << ChipID << "  row = " << row << "  Column = " << col << "   OCCCUPANCY = " << Occupancy[ChipID];



					int ChipNumber = (ChipID - ChipBoundary[lay])- sta*	NStaveChip[lay];
					if(sta == 0  && ChipID < NLay1){
						HIGMAP[ChipID]->Fill(col,row);
					}

					if(lay == 0){
						col = col + NColHis * ChipNumber;
						Lay1HIG[sta]->Fill(col,row);
					}

					if(sta == 0 && lay == 6){
						ChipIndex6 = ChipNumber/11; 
						int ChipLocal6 = ChipNumber - ChipIndex6 * 11;
						if(ChipLocal6 < 0 ) ChipLocal6 = ChipNumber - (ChipIndex6 -1) * 11;
						col = col + + ChipLocal6 * NColHis;
						HIGMAP6[ChipIndex6]->Fill(col,row);
					}


				}
				QcInfoLogger::GetInstance() << "DONE Looping the INFILE" << AliceO2::InfoLogger::InfoLogger::endm;

				TCanvas *c1 = new TCanvas ("c1", "c1", 600, 600);
				for(int j = 0; j < 1; j++){
					c1->Divide(3,3);
					for(int i = 0; i < NStaveChip[j]; i++){
						c1->cd(i+1);
						HIGMAP[i]->GetZaxis()->SetTitle("Number of Hits");
						HIGMAP[i]->GetXaxis()->SetNdivisions(-32);
						HIGMAP[i]->Draw("COLZ");
						ConfirmXAxis(HIGMAP[i]);
						ReverseYAxis(HIGMAP[i]);
						getObjectsManager()->startPublishing(HIGMAP[i]);
					}
					c1->SaveAs(Form("HIGMAPStave%d.png",j+1));
				}


				OccupancyPlot[lay]->Reset();
				LayEtaPhi[lay]->Reset();
				LayChipStave[lay]->Reset();


				QcInfoLogger::GetInstance() << "START Occupancy Filling" << AliceO2::InfoLogger::InfoLogger::endm;


				for(int i = 0; i < ChipBoundary[7]; i++){
					gm->getChipId (i, lay, sta, ssta, mod, chip);

					const Point3D<float> loc(0., 0.,0.); 
					auto glo = gm->getMatrixL2G(ChipID)(loc);

					int ChipNumber = (i - ChipBoundary[lay])- sta*	NStaveChip[lay];

					eta = glo.eta();
					phi = glo.phi();

					OccupancyPlot[lay]->Fill(Occupancy[ChipID]);
					LayEtaPhi[lay]->Fill(eta,phi,Occupancy[ChipID]);
					LayChipStave[lay]->Fill(ChipNumber,sta,Occupancy[ChipID]);
				}

				QcInfoLogger::GetInstance() << "DONE Occupancy Filling" << AliceO2::InfoLogger::InfoLogger::endm;

				TCanvas *c = new TCanvas("c","c",600,600);

				for(int j = 0; j < NLayer; j++){ 
					OccupancyPlot[j]->SetMarkerStyle(22);
					OccupancyPlot[j]->SetMarkerSize (1.5);
					OccupancyPlot[j]->Draw ("ep");
					c->SaveAs(Form("OccupancyLay%d.png",j));
					getObjectsManager()->startPublishing(OccupancyPlot[j]);
					getObjectsManager()->addMetadata(OccupancyPlot[j]->GetName(), "custom", "34");

				}

				for(int j = 0; j < NLayer; j++){ 
					LayEtaPhi[j]->Draw("COLZ");
					cout << "Eta Phi Total = " << 	LayEtaPhi[j]->Integral() << endl;
					c->SaveAs(Form("EtaPhiLay%d.png",j));
				}

				for(int j = 0; j < NLayer; j++){ 
					LayChipStave[j]->Draw("COLZ");
					cout << "LayChipStave Total = " << 	LayChipStave[j]->Integral() << endl;
					c->SaveAs(Form("LayChipStave%d.png",j));
				}


				QcInfoLogger::GetInstance() << "DONE Saving the Histogram" << AliceO2::InfoLogger::InfoLogger::endm;

				// 2. Using get("<binding>")

				// get the payload of a specific input, which is a char array. "random" is the binding specified in the config file.
				//   auto payload = ctx.inputs().get("random").payload;

				// get payload of a specific input, which is a structure array:
				//  const auto* header = header::get<header::DataHeader*>(ctx.inputs().get("random").header);
				//  struct s {int a; double b;};
				//  auto array = ctx.inputs().get<s*>("random");
				//  for (int j = 0; j < header->payloadSize / sizeof(s); ++j) {
				//    int i = array.get()[j].a;
				//  }

				// get payload of a specific input, which is a root object
				//   auto h = ctx.inputs().get<TH1F*>("histos");
				//   Double_t stats[4];
				//   h->GetStats(stats);
				//   auto s = ctx.inputs().get<TObjString*>("string");
				//   LOG(INFO) << "String is " << s->GetString().Data();
			}

			void SimpleDS::ConfirmXAxis(TH1 *h)
			{
				// Remove the current axis
				h->GetXaxis()->SetLabelOffset(999);
				h->GetXaxis()->SetTickLength(0);
				// Redraw the new axis
				gPad->Update();
				XTicks = (h->GetXaxis()->GetXmax()-h->GetXaxis()->GetXmin())/DivisionStep;

				TGaxis *newaxis = new TGaxis(gPad->GetUxmin(),
						gPad->GetUymin(),
						gPad->GetUxmax(),
						gPad->GetUymin(),
						h->GetXaxis()->GetXmin(),
						h->GetXaxis()->GetXmax(),
						XTicks,"N");
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

				YTicks = (h->GetYaxis()->GetXmax()-h->GetYaxis()->GetXmin())/DivisionStep;
				TGaxis *newaxis = new TGaxis(gPad->GetUxmin(),
						gPad->GetUymax(),
						gPad->GetUxmin()-0.001,
						gPad->GetUymin(),
						h->GetYaxis()->GetXmin(),
						h->GetYaxis()->GetXmax(),
						YTicks,"N");

				newaxis->SetLabelOffset(0);
				newaxis->Draw();
				h->GetListOfFunctions()->Add(newaxis);

			}





			void SimpleDS::endOfCycle()
			{
				QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void SimpleDS::endOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void SimpleDS::reset()
			{
				// clean all the monitor objects here

				QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
				ChipStave->Reset();
				for(int i = 0; i < NLayer; i++){
					OccupancyPlot[i]->Reset();
					LayEtaPhi[i]->Reset();
					LayChipStave[i]->Reset();
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i< NStaves[j]; i++){
						Lay1HIG[i]->Reset();
					}
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HIGMAP[i]->Reset();
					}
				}

				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HIGMAP6[i]->Reset();
					}
				}
				ErrorPlots->Reset();

				QcInfoLogger::GetInstance() << "DONE the histogram Resetting" << AliceO2::InfoLogger::InfoLogger::endm;

			}

		} // namespace simpleds
	} // namespace quality_control_modules
} // namespace o2

