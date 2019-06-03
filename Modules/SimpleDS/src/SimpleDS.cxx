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
#include <algorithm>


using o2::itsmft::Digit;



using namespace std;
using namespace o2::itsmft;
using namespace o2::ITS;



namespace o2
{
	namespace quality_control_modules
	{
		namespace simpleds
		{

			SimpleDS::SimpleDS() : TaskInterface() { 

				o2::base::GeometryManager::loadGeometry();
				gStyle->SetPadRightMargin(0.15);
				gStyle->SetPadLeftMargin(0.15);



				gStyle->SetOptFit(0);
				gStyle->SetOptStat(0);

				ChipStave->GetXaxis()->SetTitle("Chip ID");
				ChipStave->GetYaxis()->SetTitle("Number of Hits");
				ChipStave->SetTitle("Number of Hits vs Chip ID for Stave 1 at Layer 1");


				for(int i = 0; i < NLayer; i++){

					NChipLay[i] = ChipBoundary[i + 1] - ChipBoundary[i];

					OccupancyPlot[i]	= new TH1D(Form("Layer%dOccupancy",i),Form("Layer%dOccupancy",i),NEventMax[i],0,NEventMax[i]); 
					OccupancyPlot[i]->GetXaxis()->SetTitle ("Occupancy");
					OccupancyPlot[i]->GetYaxis()->SetTitle ("Counts");
					OccupancyPlot[i]->GetYaxis()->SetTitleOffset(2.2);	
					OccupancyPlot[i]->SetTitle(Form("Occupancy Distribution for ITS Layer %d",i));

					LayEtaPhi[i] = new TH2S(Form("Layer%dEtaPhi",i),Form("Layer%dEtaPhi",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhi[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhi[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhi[i]->GetZaxis()->SetTitle("Number of Hits");
					LayEtaPhi[i]->GetZaxis()->SetTitleOffset(1.4);
					LayEtaPhi[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayEtaPhi[i]->SetTitle(Form("Number of Hits for Layer %d #eta and #phi Distribution",i));


					NStaveChip[i] = NChipLay[i]/NStaves[i];
					NColStave[i] = NStaveChip[i] * NColHis;

					LayChipStave[i] = new TH2S(Form("Layer%dChipStave",i),Form("Layer%dChipStave",i),NStaveChip[i],0,NStaveChip[i],NStaves[i],0,NStaves[i]);
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
						Lay1HIT[i] = new TH2S(Form("Layer%dStave%dHITMAP",j,i),Form("Layer%dStave%dHITMAP",j,i),NColHis*NStaveChip[j],0,NColHis*NStaveChip[j],NRowHis,0,NRowHis);
						//		Lay1HIT[i] = new TH2D(Form("HICMAPLay%dStave%d",j,i),Form("HICMAPLay%dStave%d",j,i),100,0,NColHis*NStaveChip[j],100,0,NRowHis);
						Lay1HIT[i]->GetXaxis()->SetTitle("Column");
						Lay1HIT[i]->GetYaxis()->SetTitle("Row");
						Lay1HIT[i]->GetYaxis()->SetTitleOffset(1.10);
						Lay1HIT[i]->GetZaxis()->SetTitleOffset(1.50);
						Lay1HIT[i]->SetTitle(Form("Hits Map on Layer %d Stave %d",j,i));
					}
				}

				ErrorPlots->GetXaxis()->SetTitle("Error ID");
				ErrorPlots->GetYaxis()->SetTitle("Counts");
				ErrorPlots->SetTitle("Error Checked During Decoding");
				ErrorPlots->SetMinimum(0);
				cout << "DONE 1" << endl;
				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HITMAP[i]	= new TH2S(Form("Layer%dChip%dHITMAP",j,i),Form("Layer%dChip%dHITMAP",j,i),NColHis,0,NColHis,NRowHis,0,NRowHis);
						//		HITMAP[i]	= new TH2D(Form("HITMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis,100,0,NRowHis);
						HITMAP[i]->GetXaxis()->SetTitle("Column");
						HITMAP[i]->GetYaxis()->SetTitle("Row");
						HITMAP[i]->GetYaxis()->SetTitleOffset(1.10);
						HITMAP[i]->GetZaxis()->SetTitleOffset(1.50);
						HITMAP[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Number % d on Layer %d",i,j));
					}
				}
				cout << "DONE 2" << endl;

				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HITMAP6[i]	= new TH2S(Form("Layer%dStave%dHITMAP",j,i),Form("Layer%dStave%dHITMAP",j,i),NColHis*11,0,NColHis*11,NRowHis,0,NRowHis);
						//		HITMAP6[i]	= new TH2D(Form("HITMAP%dLay%d",i,j),Form("HIGMAP%dLay%d",i,j),100,0,NColHis*11,100,0,NRowHis);
						HITMAP6[i]->GetXaxis()->SetTitle("Column");
						HITMAP6[i]->GetYaxis()->SetTitle("Row");
						HITMAP6[i]->GetYaxis()->SetTitleOffset(1.10);
						HITMAP6[i]->GetZaxis()->SetTitleOffset(1.50);
						HITMAP6[i]->SetTitle(Form("Hits on Pixel of Stave 1 for Chip Sector Number % d on Layer %d",i,j));
					}
				}
				cout << "DONE 3" << endl;

				HITMAP[6]->SetMaximum(2);	
				HITMAP[6]->SetMinimum(0);	
				cout << "Clear " << endl;
				FileNameInfo->GetXaxis()->SetTitle("InputFile");
				FileNameInfo->GetYaxis()->SetTitle("Total Files Proccessed");
				FileNameInfo->GetXaxis()->SetTitleOffset(1.10);

			}

			SimpleDS::~SimpleDS() {

			}

			void SimpleDS::initialize(o2::framework::InitContext& ctx)
			{
				QcInfoLogger::GetInstance() << "initialize SimpleDS" << AliceO2::InfoLogger::InfoLogger::endm;

				o2::ITS::GeometryTGeo * geom = o2::ITS::GeometryTGeo::Instance ();
				geom->fillMatrixCache (o2::utils::bit2Mask (o2::TransformType::L2G));	
				numOfChips = geom->getNumberOfChips ();
				cout << "numOfChips = " << numOfChips << endl;
				setNChips (numOfChips);
				cout << "START LOOPING BR getObjectsManager()->startPublishingO" << endl;



				cout << "START Inititing Publication " << endl;


				getObjectsManager()->startPublishing(FileNameInfo);


				ChipStave->SetMinimum(1);
				getObjectsManager()->startPublishing(ChipStave);

				for(int i =0; i< NError;i++){
					pt[i] = new TPaveText(0.20,0.80 -i*0.05,0.85,0.85-i*0.05,"NDC");
					pt[i]->SetTextSize(0.04);
					pt[i]->SetFillColor(0);
					pt[i]->SetTextAlign(12);
					pt[i]->AddText(ErrorType[i].Data());
					ErrorPlots->GetListOfFunctions()->Add(pt[i]);
				}

				//TLegend* l = new TLegend(0.15,0.50,0.90,0.90);
				ErrorMax = ErrorPlots->GetMaximum();

				cout << "ErrorMax = " << ErrorMax << endl;

				ErrorPlots->SetMaximum(ErrorMax * 4.1+1000);
				//	ErrorPlots->SetName(Form("%s-%s",ErrorPlots->GetName(),HisRunID.Data()));
				//	cout << "ErrorPlot Name = " << ErrorPlots->GetName() << endl;
				getObjectsManager()->startPublishing(ErrorPlots);
				getObjectsManager()->addMetadata(ErrorPlots->GetName(), "custom", "34");



				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HITMAP[i]->GetZaxis()->SetTitle("Number of Hits");
						HITMAP[i]->GetXaxis()->SetNdivisions(-32);
						HITMAP[i]->Draw("COLZ");
						ConfirmXAxis(HITMAP[i]);
						ReverseYAxis(HITMAP[i]);
						getObjectsManager()->startPublishing(HITMAP[i]);
					}
				}


				ReverseYAxis(HITMAP[0]);

				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaves[j]; i++){

						Lay1HIT[i]->GetZaxis()->SetTitle("Number of Hits");
						Lay1HIT[i]->GetXaxis()->SetNdivisions(-32);
						Lay1HIT[i]->Draw("COLZ");
						ConfirmXAxis(Lay1HIT[i]);
						ReverseYAxis(Lay1HIT[i]);
						getObjectsManager()->startPublishing(Lay1HIT[i]);
					}
				}





				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HITMAP6[i]->GetZaxis()->SetTitle("Number of Hits");
						HITMAP6[i]->GetXaxis()->SetNdivisions(-32);
						HITMAP6[i]->Draw("COLZ");
						ConfirmXAxis(HITMAP6[i]);
						ReverseYAxis(HITMAP6[i]);	
						getObjectsManager()->startPublishing(HITMAP6[i]);
					}
				}




				for(int i = 0; i < NLayer; i++){

					getObjectsManager()->startPublishing(LayEtaPhi[i]);
					getObjectsManager()->startPublishing(LayChipStave[i]);
					getObjectsManager()->startPublishing(OccupancyPlot[i]);

				}

				cout << "DONE Inititing Publication = " << endl;


				RunIDPre = 0;
				FileIDPre = 0;


			}

			void SimpleDS::startOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;

			}

			void SimpleDS::startOfCycle()
			{
				QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void SimpleDS::monitorData(o2::framework::ProcessingContext& ctx)
			{



				QcInfoLogger::GetInstance() << "BEEN HERE BRO" << AliceO2::InfoLogger::InfoLogger::endm;
				

				
				//For The Moment//
				
				int RunID = ctx.inputs().get<int>("Run");
				int FileID = ctx.inputs().get<int>("File");
				//QcInfoLogger::GetInstance() << "RunID IN QC = "  << runID;


				TString RunName = Form("Run%d",RunID);
				TString FileName = Form("infile/Run%d/data-link%d",RunID,FileID);

				if(RunIDPre != RunID || FileIDPre != FileID){
				QcInfoLogger::GetInstance() << "For the Moment: RunID = "  << RunID << "  FileID = " << FileID << AliceO2::InfoLogger::InfoLogger::endm;
				FileNameInfo->Fill(0.5);
				FileNameInfo->SetTitle(Form("Current File Name: %s",FileName.Data()));
				}
				RunIDPre = RunID;
				FileIDPre = FileID;

				//Will Fix Later//
				

				int ResetDecision = ctx.inputs().get<int>("in");
				QcInfoLogger::GetInstance() << "Reset Histogram Decision = " << ResetDecision << AliceO2::InfoLogger::InfoLogger::endm;
				if(ResetDecision == 1) reset();

				std::array<unsigned int,NError> Errors = ctx.inputs().get<const std::array<unsigned int,NError>>("Error");

				for(int i = 0; i < NError; i++){
					QcInfoLogger::GetInstance() << " i = " << i << "   Error = "	 << Errors[i]  <<  AliceO2::InfoLogger::InfoLogger::endm;
					ErrorPlots->SetBinContent(i+1,Errors[i]);
				}


				/*
				   Error[0] = ctx.inputs().get<int>("Error0");
				   QcInfoLogger::GetInstance() << "Errorvec 0 = "  << 	Error[0]  <<  AliceO2::InfoLogger::InfoLogger::endm;
				   Error[1] = ctx.inputs().get<int>("Error1");
				   QcInfoLogger::GetInstance() << "Errorvec 1 = "  << 	Error[1]  <<  AliceO2::InfoLogger::InfoLogger::endm;
				   Error[2] = ctx.inputs().get<int>("Error2");
				   QcInfoLogger::GetInstance() << "Errorvec 2 = "  << 	Error[2]  <<  AliceO2::InfoLogger::InfoLogger::endm;
				   Error[3] = ctx.inputs().get<int>("Error3");
				   QcInfoLogger::GetInstance() << "Errorvec 3 = "  << 	Error[3]  <<  AliceO2::InfoLogger::InfoLogger::endm;
				   Error[4] = ctx.inputs().get<int>("Error4");
				   QcInfoLogger::GetInstance() << "Errorvec 4 = "  << 	Error[4]  <<  AliceO2::InfoLogger::InfoLogger::endm;
				   Error[5] = ctx.inputs().get<int>("Error5");
				   QcInfoLogger::GetInstance() << "Errorvec 5 = "  << 	Error[5]  <<  AliceO2::InfoLogger::InfoLogger::endm;
				   Error[6] = ctx.inputs().get<int>("Error6");
				   QcInfoLogger::GetInstance() << "Errorvec 6 = "  << 	Error[6]  <<  AliceO2::InfoLogger::InfoLogger::endm;
				   */
				//Error[7] = ctx.inputs().get<int>("Error7");
				//QcInfoLogger::GetInstance() << "Errorvec 7 = "  << 	Error[7]  <<  AliceO2::InfoLogger::InfoLogger::endm;
				//Error[8] = ctx.inputs().get<int>("Error8");
				//QcInfoLogger::GetInstance() << "Errorvec 8 = "  << 	Error[8]  <<  AliceO2::InfoLogger::InfoLogger::endm;

				//		Error[9] = ctx.inputs().get<int>("Error9");
				//		QcInfoLogger::GetInstance() << "Errorvec 9 = "  << 	Error[9]  <<  AliceO2::InfoLogger::InfoLogger::endm;




				auto digits = ctx.inputs().get<const std::vector<o2::itsmft::Digit>>("digits");
				LOG(INFO) << "Digit Size Getting For This TimeFrame (Event) = " <<  digits.size();


				for (auto&& pixeldata : digits) {



					ChipID = pixeldata.getChipIndex();
					col = pixeldata.getColumn();
					row = pixeldata.getRow();
					NEvent = pixeldata.getCharge();


					if (NEvent%10000==0 && NEvent > 0) cout << "ChipID = " << ChipID << "  col = " << col << "  row = " << row << "  NEvent = " << NEvent << endl;



					gm->getChipId (ChipID, lay, sta, ssta, mod, chip);
					gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
					const Point3D<float> loc(0., 0.,0.); 
					auto glo = gm->getMatrixL2G(ChipID)(loc);



					if(ChipID != ChipIDPre){
						OccupancyPlot[lay]->Fill(OccupancyCounter);
						OccupancyCounter = 0;
					}
					OccupancyCounter  = OccupancyCounter + 1;


					if (lay < 7)
					{
						//cout << "lay = " <<  lay << endl;
						//cout << "ChipID = " << ChipID << endl;

						//Layer Occupancy Plot//


						int ChipNumber = (ChipID - ChipBoundary[lay])- sta*	NStaveChip[lay];

						LayEtaPhi[lay]->Fill(eta,phi);
						LayChipStave[lay]->Fill(ChipNumber,sta);


						if(sta == 0  && ChipID < NLay1){
							if(row > 0 && col > 0) HITMAP[ChipID]->Fill(col,row);
						}



						eta = glo.eta();
						phi = glo.phi();
						if(lay == 0){
							//cout << "ChipID in Stave 0 = " << ChipID << endl; 
							rowCS = row;
							colCS = col + NColHis * ChipNumber;
							if(row > 0 && col > 0) Lay1HIT[sta]->Fill(rowCS,colCS);
						}

						if(sta == 0 && lay == 6){
							ChipIndex6 = ChipNumber/11; 
							int ChipLocal6 = ChipNumber - ChipIndex6 * 11;
							if(ChipLocal6 < 0 ) ChipLocal6 = ChipNumber - (ChipIndex6 -1) * 11;
							rowLay6 = row;
							colLay6 = col + ChipLocal6 * NColHis;
							if(row > 0 && col > 0) HITMAP6[ChipIndex6]->Fill(rowLay6,colLay6);	
						}

					}

					NEventPre = NEvent;
					ChipIDPre = ChipID;
				}
				/*

				   for(int i = 0; i < NEvent; i++){
				   for(int j = 0; j < numOfChips; i++){
				   gm->getChipId (j, lay, sta, ssta, mod, chip);
				   if(Frequency[i][j] > 0) OccupancyPlot[lay]->Fill(Frequency[i][j]);
				   }	

				   }
				   */

				digits.clear();


				//	o2::ITSMFT::Digit digit = ctx.inputs().get<o2::ITSMFT::Digit>("digits");
				//	LOG(INFO) << "Chip ID Getting " << digit.getChipIndex() << " Row = " << digit.getRow() << "   Column = " << digit.getColumn();

				/*	
					ChipID = digit.getChipIndex();
					col = digit.getColumn();
					row = digit.getRow();

					gm->getChipId (ChipID, lay, sta, ssta, mod, chip);
					gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
					Occupancy[ChipID] = Occupancy[ChipID] + 1;

				//			LOG(INFO) << "ChipID = " << ChipID << "  row = " << row << "  Column = " << col << "   OCCCUPANCY = " << Occupancy[ChipID];


				if(TotalDigits%1000==0) 	LOG(INFO) << "TotalDigits = " << TotalDigits  << "   ChipID = " << ChipID;

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
				*/

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
				ChipStave->Reset();
				for(int i = 0; i < NLayer; i++){
					OccupancyPlot[i]->Reset();
					LayEtaPhi[i]->Reset();
					LayChipStave[i]->Reset();
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i< NStaves[j]; i++){
						Lay1HIT[i]->Reset();
					}
				}

				for(int j = 0; j < 1; j++){
					for(int i = 0; i < NStaveChip[j]; i++){
						HITMAP[i]->Reset();
					}
				}

				for(int j = 6; j < 7; j++){
					for(int i = 0; i < 18; i++){
						HITMAP6[i]->Reset();
					}
				}
				ErrorPlots->Reset();
				NEventInRun = 0;

				QcInfoLogger::GetInstance() << "DONE the histogram Resetting" << AliceO2::InfoLogger::InfoLogger::endm;

			}

		} // namespace simpleds
	} // namespace quality_control_modules
} // namespace o2

