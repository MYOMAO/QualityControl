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
				o2::base::GeometryManager::loadGeometry ();
				gStyle->SetOptFit(0);
				gStyle->SetOptStat(0);	
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

				for (auto&& input : ctx.inputs()) {
					o2::ITSMFT::Digit digit = ctx.inputs().get<o2::ITSMFT::Digit>("digits");
					LOG(INFO) << "Chip ID Getting " << digit.getChipIndex() << " Row = " << digit.getRow() << "   Column = " << digit.getColumn();
				}

				QcInfoLogger::GetInstance() << "DONE" << AliceO2::InfoLogger::InfoLogger::endm;

				/*
				for (auto&& input : ctx.inputs()) {
					o2::ITSMFT::Digit digit = ctx.inputs().get<o2::ITSMFT::Digit>("digits");
					LOG(INFO) << "Chip ID Getting " << digit.getChipIndex() << " Row = " << digit.getRow() << "   Column = " << digit.getColumn();
					gm->getChipId (ChipID, lay, sta, ssta, mod, chip);
					gm->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
					const Point3D<float> loc(0., 0.,0.); 
					auto glo = gm->getMatrixL2G(ChipID)(loc);
					int ChipNumber = (ChipID - ChipBoundary[lay])- sta*	NStaveChip[lay];
					if(sta == 0  && ChipID < NLay1){
						HIGMAP[ChipID]->Fill(col,row);
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

				QcInfoLogger::GetInstance() << "DONE Saving the Histogram" << AliceO2::InfoLogger::InfoLogger::endm;
				*/

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
			}

		} // namespace simpleds
	} // namespace quality_control_modules
} // namespace o2

