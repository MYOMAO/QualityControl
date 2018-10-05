///
/// \file   CheckerDataProcessorFactory.cxx
/// \author Piotr Konopka
///

#include <Framework/DataProcessorSpec.h>

#include "QualityControl/CheckerDataProcessorFactory.h"
#include "QualityControl/CheckerDataProcessor.h"

namespace o2 {
namespace quality_control {
namespace checker {

using namespace o2::framework;
using namespace o2::quality_control::checker;

CheckerDataProcessorFactory::CheckerDataProcessorFactory()
{
}

CheckerDataProcessorFactory::~CheckerDataProcessorFactory()
{
}

DataProcessorSpec CheckerDataProcessorFactory::create(std::string checkerName, std::string taskName, std::string configurationSource)
{
  CheckerDataProcessor qcChecker{ checkerName, taskName, configurationSource };

  DataProcessorSpec newChecker{
    checkerName,
    Inputs{ qcChecker.getInputSpec() },
    Outputs{ qcChecker.getOutputSpec() },
    adaptFromTask<CheckerDataProcessor>(std::move(qcChecker)),
    Options{},
    std::vector<std::string>{},
    std::vector<DataProcessorLabel>{}
  };

  return std::move(newChecker);
}

} // namespace checker
} // namespace quality_control
} // namespace o2