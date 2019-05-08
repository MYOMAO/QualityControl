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
				o2::base::GeometryManager::loadGeometry ();


				gStyle->SetOptFit(0);
				gStyle->SetOptStat(0);


				mHistogram = nullptr; 
				o2::base::GeometryManager::loadGeometry ();
				geom->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::T2L));

				for(int i = 0; i < NLayer; i++){

					LayEtaPhiClus[i] = new TH2D(Form("ClusEtaPhiLay%d",i),Form("ClusEtaPhiLay%d",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhiClus[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhiClus[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhiClus[i]->GetZaxis()->SetTitle("Cluster Size");
					LayEtaPhiClus[i]->GetZaxis()->SetTitleOffset(1.4);
					LayEtaPhiClus[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayEtaPhiClus[i]->SetTitle(Form("Cluster Size for Layer %d #eta and #phi Distribution",i));
					
					LayClusDis[i] = new TH1D(Form("ClusDisLay%d",i),Form("ClusDisLay%d",i),NClusBin,0,NClusMax);
					LayClusDis[i]->GetXaxis()->SetTitle("Cluster Size");
					LayClusDis[i]->GetYaxis()->SetTitle("Counts");
					LayClusDis[i]->SetTitle(Form("Cluster Size Distribution for Layer %d",i));
					LayClusDis[i]->GetYaxis()->SetTitleOffset(1.40);	


					LayEtaPhiClusNum[i] = new TH2D(Form("ClusNumEtaPhiLay%d",i),Form("ClusNumEtaPhiLay%d",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhiClusNum[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhiClusNum[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhiClusNum[i]->GetZaxis()->SetTitle("Number of Clusters");
					LayEtaPhiClusNum[i]->GetZaxis()->SetTitleOffset(1.4);
					LayEtaPhiClusNum[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayEtaPhiClusNum[i]->SetTitle(Form("Cluster Number for Layer %d #eta and #phi Distribution",i));
				
					LayClusNumDis[i] = new TH1D(Form("ClusNumDisLay%d",i),Form("ClusNumDisLay%d",i),NClusNumBin,0,NClusNumMax);
					LayClusNumDis[i]->GetXaxis()->SetTitle("Number of Clusters");
					LayClusNumDis[i]->GetYaxis()->SetTitle("Counts");
					LayClusNumDis[i]->SetTitle(Form("Cluster Number Distribution for Layer %d",i));
					LayClusNumDis[i]->GetYaxis()->SetTitleOffset(1.40);	


					LayEtaPhiClusID[i] = new TH2D(Form("ClusIDEtaPhiLay%d",i),Form("ClusIDEtaPhiLay%d",i),NEta,EtaMin,EtaMax,NPhi,PhiMin,PhiMax);
					LayEtaPhiClusID[i]->GetXaxis()->SetTitle("#eta");
					LayEtaPhiClusID[i]->GetYaxis()->SetTitle("#phi");
					LayEtaPhiClusID[i]->GetZaxis()->SetTitle("Clusters Pattern ID");
					LayEtaPhiClusID[i]->GetZaxis()->SetTitleOffset(1.4);
					LayEtaPhiClusID[i]->GetYaxis()->SetTitleOffset(1.10);	
					LayEtaPhiClusID[i]->SetTitle(Form("Cluster Pattern ID for Layer %d #eta and #phi Distribution",i));


			
					LayClusIDDis[i] = new TH1D(Form("ClusIDDisLay%d",i),Form("ClusIDDisLay%d",i),NClusIDBin,0,NClusIDMax);
					LayClusIDDis[i]->GetXaxis()->SetTitle("Cluster Pattern ID");
					LayClusIDDis[i]->GetYaxis()->SetTitle("Counts");
					LayClusIDDis[i]->SetTitle(Form("Cluster Pattern ID Distribution for Layer %d",i));
					LayClusIDDis[i]->GetYaxis()->SetTitleOffset(1.40);	


				}

			}

			ITSQCCluster::~ITSQCCluster() {
				if (mHistogram) {
					delete mHistogram;
				}
			}

			void ITSQCCluster::initialize(o2::framework::InitContext& ctx,std::string infile)
			{

				QcInfoLogger::GetInstance() << "initialize ITSQCCluster" << AliceO2::InfoLogger::InfoLogger::endm;
				QcInfoLogger::GetInstance() << "infile is = " << infile << AliceO2::InfoLogger::InfoLogger::endm;
				clusterinfile = infile;
				QcInfoLogger::GetInstance() << "clusterinfile is = " << clusterinfile << AliceO2::InfoLogger::InfoLogger::endm;

				TChain itsClusters("o2sim");
				itsClusters.AddFile(clusterinfile.Data());

				if (!itsClusters.GetBranch("ITSCluster")) {
					LOG(FATAL) << "Did not find ITS clusters branch ITSCluster in the input tree" << FairLogger::endl;
				}
				std::vector<o2::ITSMFT::Cluster>* clusters = nullptr;
		     	std::vector<o2::ITSMFT::CompClusterExt> *CompClus = nullptr;

				itsClusters.SetBranchAddress("ITSCluster", &clusters);
				itsClusters.SetBranchAddress("ITSClusterComp", &CompClus);

				NEvents = itsClusters.GetEntries();
				NEvents = 1;


				for (int iEvent = 0; iEvent < NEvents; iEvent++) {
					itsClusters.GetEntry(iEvent);
					TotalClusterSize = clusters->size();

					cout << "TotalClusterSize = " <<  TotalClusterSize << endl;
					for(int i = 0; i< TotalClusterSize; i++){
						Cluster& c = (*clusters)[i];
						CompClusterExt & cc = (*CompClus)[i];

						ChipID = c.getSensorID();
						//int Byte = c.getBits();
						//int Count = c.getCount();

						// cout << "Byte = " << Byte << "  Count = " << Count <<  endl;
						Clusx = c.getX();
						Clusy = c.getY();
						Clusz = c.getZ();
						ClusSize = c.getNPix();
						//cout << "Pass Here" << endl;

						ClusterID = cc.getPatternID();
						
	
						//cout << "X = " <<  Clusx << "  Y = " << Clusy <<  "   Z = " << Clusz << "  Cluster Size = " <<  ClusSize << "   ClusterID = "  <<  cc.getPatternID() << endl;
				

						Clusphi = TMath::ATan(Clusy/Clusx);
						Clustheta = TMath::ATan(TMath::Sqrt(Clusx*Clusx + Clusy*Clusy)/TMath::Abs(Clusz));
						Cluseta = -TMath::Log(TMath::Tan(Clustheta/2));



					//	cout << "Eta = " <<  Cluseta << "  Phi = " << Clusphi <<  "  Cluster Size = " <<  ClusSize << endl;

						geom->getChipId (ChipID, lay, sta, ssta, mod, chip);
						geom->fillMatrixCache(o2::utils::bit2Mask(o2::TransformType::L2G));
						const Point3D<float> loc(0., 0.,0.); 
						auto glo = geom->getMatrixL2G(ChipID)(loc);
						eta = glo.eta();
						phi = glo.phi();


						LayEtaPhiClus[lay]->Fill(Cluseta,Clusphi,ClusSize);
						LayClusDis[lay]->Fill(ClusSize);
						LayEtaPhiClusNum[lay]->Fill(Cluseta,Clusphi,1);
						LayEtaPhiClusID[lay]->Fill(Cluseta,Clusphi,ClusterID);	
						LayClusIDDis[lay]->Fill(ClusterID);	

					}


				}

				TCanvas * c1 = new TCanvas("c1","c1",600,600);
				for(int j = 0; j < NLayer; j++){ 
					LayEtaPhiClus[j]->Draw("COLZ");
					cout << "Eta Phi Total = " << 	LayEtaPhiClus[j]->Integral() << endl;
					c1->SaveAs(Form("Cluster/EtaPhiLayClus%d.png",j));
				}
		

				for(int j = 0; j < NLayer; j++){ 
					LayEtaPhiClusNum[j]->Draw("COLZ");
					cout << "Eta Phi Total = " << 	LayEtaPhiClusNum[j]->Integral() << endl;
					c1->SaveAs(Form("Cluster/LayEtaPhiClusNum%d.png",j));
				}

				
				for(int j = 0; j < NLayer; j++){ 
					LayEtaPhiClusID[j]->Draw("COLZ");
					cout << "Eta Phi Total = " << 	LayEtaPhiClusNum[j]->Integral() << endl;
					c1->SaveAs(Form("Cluster/LayEtaPhiClusID%d.png",j));
				}

				c1->SetLogy();

				for(int j = 0; j < NLayer; j++){ 
					LayClusDis[j]->Draw();
					cout << "Clus Dis Lay " <<  LayClusDis[j]->Integral() << endl;
					c1->SaveAs(Form("Cluster/LayClusDis%d.png",j));
				}


				for(int j = 0; j < NLayer; j++){ 
					LayClusIDDis[j]->Draw();
					cout << "Clus Dis ID Lay " <<  	LayClusIDDis[j]->Integral() << endl;
					c1->SaveAs(Form("Cluster/LayClusIDDis%d.png",j));
				}

				for(int i = 0; i < NLayer; i++){
					getObjectsManager()->startPublishing(LayEtaPhiClus[i]);
					getObjectsManager()->startPublishing(LayClusDis[i]);
					getObjectsManager()->startPublishing(LayEtaPhiClusNum[i]);
					getObjectsManager()->startPublishing(LayEtaPhiClusID[i]);
					getObjectsManager()->startPublishing(LayClusIDDis[i]);

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

