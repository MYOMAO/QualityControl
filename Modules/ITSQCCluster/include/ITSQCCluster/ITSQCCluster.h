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

#include "../../../../O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/PixelReader.h"
#include "../../../../O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/PixelData.h"
#include "../../../../O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/LookUp.h"
#include "../../../../O2/DataFormats/Detectors/ITSMFT/common/include/DataFormatsITSMFT/Cluster.h"
#include "../../../../O2/DataFormats/Detectors/ITSMFT/common/include/DataFormatsITSMFT/CompCluster.h"
#include "../../../../O2/DataFormats/Detectors/ITSMFT/common/include/DataFormatsITSMFT/ROFRecord.h"

#include "../../../../O2/DataFormats/simulation/include/SimulationDataFormat/MCCompLabel.h"
#include "../../../../O2/DataFormats/simulation/include/SimulationDataFormat/MCTruthContainer.h"
#include "../../../../O2/DataFormats/Reconstruction/include/ReconstructionDataFormats/BaseCluster.h"

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
					void initialize(o2::framework::InitContext& ctx) override;
					void startOfActivity(Activity& activity) override;
					void startOfCycle() override;
					void monitorData(o2::framework::ProcessingContext& ctx) override;
					void endOfCycle() override;
					void endOfActivity(Activity& activity) override;
					void reset() override;

				private:
					static constexpr int NLayer = 7;
					TH1F* mHistogram;
					int TotalClusterSize;
					double ClusterPosX;
					TString clusterinfile = "o2clus_its.root";
					o2::ITS::GeometryTGeo* geom = o2::ITS::GeometryTGeo::Instance();
					int ChipID;
					TH2D * LayEtaPhiClus[NLayer]; 
					int lay, sta, ssta, mod, chip;		
					const int NEta = 9;
					const double EtaMin = -2.40;
					const double EtaMax = 2.40;
					const int NPhi = 12;
					const double PhiMin = -2.90;
					const double PhiMax = 2.90;
					const int NChipsSta = 9;
					double eta;
					double phi;
			};

		} // namespace itsqccluster
	} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_ITSQCCLUSTER_ITSQCCLUSTER_H

