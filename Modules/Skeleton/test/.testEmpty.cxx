///
/// \file   .testEmpty.cxx
/// \author
///

#include "QualityControl/TaskFactory.h"

#define BOOST_TEST_MODULE Publisher test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

namespace o2
{
namespace quality_control_modules
{
namespace skeleton
{

BOOST_AUTO_TEST_CASE(instantiate_task) { BOOST_CHECK(true); }

} // namespace skeleton
} // namespace quality_control_modules
} // namespace o2
