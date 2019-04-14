///
/// \file   ITSQCCluster.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "ITSQCCluster/ITSQCCluster.h"

#include <sstream>

#include <TStopwatch.h>
#include "DataFormatsParameters/GRPObject.h"
#include "FairLogger.h"
#include "FairRunAna.h"
#include "FairFileSource.h"
#include "FairRuntimeDb.h"
#include "FairParRootFileIo.h"
#include "FairSystemInfo.h"

#include "ITSMFTReconstruction/DigitPixelReader.h"
#include "DetectorsBase/GeometryManager.h"
#include <TCanvas.h>
#include "ITSBase/GeometryTGeo.h"


using o2::ITSMFT::Digit;



using namespace std;
using namespace o2::ITSMFT;
using namespace o2::ITS;




namespace o2
{
	namespace quality_control_modules
	{
		namespace itsqccluster
		{

			ITSQCCluster::ITSQCCluster() : TaskInterface(), mHistogram(nullptr) { 
		
				gStyle->SetPadRightMargin(0.15);
				gStyle->SetPadLeftMargin(0.15);
				o2::Base::GeometryManager::loadGeometry ();


				gStyle->SetOptFit(0);
				gStyle->SetOptStat(0);


				mHistogram = nullptr; 
				o2::Base::GeometryManager::loadGeometry ();
				geom->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::T2L));

				for(int i = 0; i < NLayer; i++){

					LayEtaPhiClus[i] = new TH2D(Form("Lay1EtaPhiLay%d",i),Form("Lay1EtaPhiLay%d",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhiClus[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhiClus[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhiClus[i]->GetZaxis()->SetTitle("Cluster Size");
					LayEtaPhiClus[i]->GetZaxis()->SetTitleOffset(1.4);
					LayEtaPhiClus[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayEtaPhiClus[i]->SetTitle(Form("Cluster Size for Layer %d #eta and #phi Distribution",i));
				}

			}

			ITSQCCluster::~ITSQCCluster() {
				if (mHistogram) {
					delete mHistogram;
				}
			}

			void ITSQCCluster::initialize(o2::framework::InitContext& ctx)
			{

				QcInfoLogger::GetInstance() << "initialize ITSQCCluster" << AliceO2::InfoLogger::InfoLogger::endm;

				TChain itsClusters("o2sim");
				itsClusters.AddFile(clusterinfile.Data());

				if (!itsClusters.GetBranch("ITSCluster")) {
					LOG(FATAL) << "Did not find ITS clusters branch ITSCluster in the input tree" << FairLogger::endl;
				}
				std::vector<o2::ITSMFT::Cluster>* clusters = nullptr;
				itsClusters.SetBranchAddress("ITSCluster", &clusters);



				for (int iEvent = 0; iEvent < itsClusters.GetEntries(); ++iEvent) {
					itsClusters.GetEntry(iEvent);
					TotalClusterSize = clusters->size();

					cout << "TotalClusterSize = " <<  TotalClusterSize << endl;
					for(int i = 0; i< TotalClusterSize; i++){
						Cluster& c = (*clusters)[i];
						ChipID = c.getSensorID();

						//	 cout << "ChipID = " << ChipID << endl;

						geom->getChipId (ChipID, lay, sta, ssta, mod, chip);
						geom->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
						const Point3D<float> loc(0., 0.,0.); 
						auto glo = geom->getMatrixL2G(ChipID)(loc);
						eta = glo.eta();
						phi = glo.phi();
						LayEtaPhiClus[lay]->Fill(eta,phi);

					}

				}

				TCanvas * c1 = new TCanvas("c1","c1",600,600);
				for(int j = 0; j < NLayer; j++){ 
					LayEtaPhiClus[j]->Draw("COLZ");
					cout << "Eta Phi Total = " << 	LayEtaPhiClus[j]->Integral() << endl;
					c1->SaveAs(Form("EtaPhiLayClus%d.png",j));
				}



				for(int i = 0; i < NLayer; i++){
					getObjectsManager()->startPublishing(LayEtaPhiClus[i]);
				}



				mHistogram = new TH1F("example", "example", 20, 0, 30000);
				//getObjectsManager()->startPublishing(mHistogram);
				//getObjectsManager()->addCheck(mHistogram, "checkFromITSQCCluster", "o2::quality_control_modules::itsqccluster::ITSQCClusterCheck","QcITSQCCluster");

			}

			void ITSQCCluster::startOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

			void ITSQCCluster::startOfCycle()
			{
				QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSQCCluster::monitorData(o2::framework::ProcessingContext& ctx)
			{

			}

			void ITSQCCluster::endOfCycle()
			{
				QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSQCCluster::endOfActivity(Activity& activity)
			{
				QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
			}

			void ITSQCCluster::reset()
			{
				// clean all the monitor objects here

				QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
				mHistogram->Reset();
			}

		} // namespace itsqccluster
	} // namespace quality_control_modules
} // namespace o2

