///
/// \file   ITSQCCluster.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_MODULE_ITSQCCLUSTER_ITSQCCLUSTER_H
#define QC_MODULE_ITSQCCLUSTER_ITSQCCLUSTER_H

#include "QualityControl/TaskInterface.h"

#include <utility>
#include <vector>
#include <cstring>
#include "ITSMFTBase/GeometryTGeo.h"
#include "ITSMFTBase/SegmentationAlpide.h"
#include "Rtypes.h"
#include "TTree.h"
#include "ITSBase/GeometryTGeo.h"
#include "DetectorsBase/GeometryManager.h"
#include "uti.h"
#include "DataFormatsITSMFT/CompCluster.h"

#include "ITSMFTReconstruction/PixelReader.h"
#include "ITSMFTReconstruction/PixelData.h"
#include "ITSMFTReconstruction/LookUp.h"
#include "DataFormatsITSMFT/Cluster.h"
#include "DataFormatsITSMFT/CompCluster.h"
#include "DataFormatsITSMFT/ROFRecord.h"

#include "SimulationDataFormat/MCCompLabel.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "ReconstructionDataFormats/BaseCluster.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2
{
	namespace quality_control_modules
	{
		namespace itsqccluster
		{

			/// \brief Example Quality Control DPL Task
			/// It is final because there is no reason to derive from it. Just remove it if needed.
			/// \author Barthelemy von Haller
			/// \author Piotr Konopka
			class ITSQCCluster /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
			{
				public:
					/// \brief Constructor
					ITSQCCluster();
					/// Destructor
					~ITSQCCluster() override;

					// Definition of the methods for the template method pattern
			//		void initialize(o2::framework::InitContext& ctx) override;
					void initialize(o2::framework::InitContext& ctx, std::string infile) override;
					void startOfActivity(Activity& activity) override;
					void startOfCycle() override;
					void monitorData(o2::framework::ProcessingContext& ctx) override;
					void endOfCycle() override;
					void endOfActivity(Activity& activity) override;
					void reset() override;

				private:
					static constexpr int NLayer = 7;
					int NEvents;
					TH1F* mHistogram;
					int TotalClusterSize;
					double ClusterPosX;
					TString clusterinfile = "o2clus_its.root";
					o2::ITS::GeometryTGeo* geom = o2::ITS::GeometryTGeo::Instance();
					int ChipID;
					TH1D * LayClusDis[NLayer]; 	
					TH2D * LayEtaPhiClus[NLayer]; 
					TH1D * LayClusNumDis[NLayer]; 	
					TH2D * LayEtaPhiClusNum[NLayer]; 
					TH1D * LayClusIDDis[NLayer]; 	
					TH2D * LayEtaPhiClusID[NLayer]; 

					int lay, sta, ssta, mod, chip;		
					const int NEta = 500;
					const double EtaMin = 0;
					const double EtaMax = 5;
					const int NPhi = 100;
					const double PhiMin = 0.4;
					const double PhiMax = 0.6;
					const int NChipsSta = 9;
					double eta;
					double phi;
					double Clusx;
					double Clusy;
					double Clusz;
					int ClusSize;
					double Cluseta;
					double Clustheta;	
					double Clusphi;
					int NClusStep = 1;
					int NClusMax = 20;
					int NClusBin = NClusMax/NClusStep;
	
					int NClusNumStep = 1;
					int NClusNumMax = 5;
					int NClusNumBin = NClusNumMax/NClusNumStep;

					int NClusIDStep = 1;
					int NClusIDMax = 50;
					int NClusIDBin = NClusIDMax/NClusIDStep;


					int ClusterNumber;
					int ClusterID;


			};

		} // namespace itsqccluster
	} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_ITSQCCLUSTER_ITSQCCLUSTER_H

