//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/ParInstrument.h"

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CheckWorkspacesMatch)

using namespace Kernel;
using namespace API;

void CheckWorkspacesMatch::init()
{
  declareProperty(new WorkspaceProperty<>("Workspace1","",Direction::Input));
  declareProperty(new WorkspaceProperty<>("Workspace2","",Direction::Input));

  declareProperty("Tolerance",0.0);
  
  declareProperty("CheckAxes",true);
  declareProperty("CheckSpectraMap",true);
  declareProperty("CheckInstrument",true);
  declareProperty("CheckMasking",true);
  
  declareProperty("Result","",Direction::Output);
}

void CheckWorkspacesMatch::exec()
{
  result.clear();
  this->doComparison();
  
  if ( result != "") 
  {
    g_log.notice() << "The workspaces did not match: " << result << std::endl;
  }
  else
  {
    result = "Success!";
  }
  setProperty("Result",result);
  
  return;
}

void CheckWorkspacesMatch::doComparison()
{
  MatrixWorkspace_const_sptr ws1 = getProperty("Workspace1");
  MatrixWorkspace_const_sptr ws2 = getProperty("Workspace2");

  // First check the data - always do this
  if ( ! checkData(ws1,ws2) ) return;
  
  // Now do the other ones if requested. Bail out as soon as we see a failure.
  if ( getProperty("CheckAxes") && ! checkAxes(ws1,ws2) ) return;
  if ( getProperty("CheckSpectraMap") && ! checkSpectraMap(ws1,ws2) ) return;
  if ( getProperty("CheckInstrument") && ! checkInstrument(ws1,ws2) ) return;
  if ( getProperty("CheckMasking") && ! checkMasking(ws1,ws2) ) return;
  
  return;
}

bool CheckWorkspacesMatch::checkData(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
{
  // Cache a few things for later use
  const int numHists = ws1->getNumberHistograms();
  const int numBins = ws1->blocksize();
  const bool histogram = ws1->isHistogramData();
  
  // First check that the workspace are the same size
  if ( numHists != ws2->getNumberHistograms() || numBins != ws2->blocksize() )
  {
    result = "Size mismatch";
    return false;
  }
  
  // Check that both are either histograms or point-like data
  if ( histogram != ws2->isHistogramData() )
  {
    result = "Histogram/point-like mismatch";
    return false;
  }
  
  // Check both have the same distribution flag
  if ( ws1->isDistribution() != ws2->isDistribution() )
  {
    result = "Distribution flag mismatch";
    return false;
  }
  
  const double tolerance = getProperty("Tolerance");
  
  // Now check the data itself
  for ( int i = 0; i < numHists; ++i )
  {
    // Get references to the current spectrum
    const MantidVec& X1 = ws1->readX(i);
    const MantidVec& Y1 = ws1->readY(i);
    const MantidVec& E1 = ws1->readE(i);
    const MantidVec& X2 = ws2->readX(i);
    const MantidVec& Y2 = ws2->readY(i);
    const MantidVec& E2 = ws2->readE(i);
    
    for ( int j = 0; j < numBins; ++j )
    {
      if ( std::abs(X1[j]-X2[j]) > tolerance || std::abs(Y1[j]-Y2[j]) > tolerance || std::abs(E1[j]-E2[j]) > tolerance ) 
      {
        result = "Data mismatch";
        return false;
      }
    }
    
    // Extra one for histogram data
    if ( histogram && std::abs(X1.back()-X2.back()) > tolerance ) 
    {
      result = "Data mismatch";
      return false;
    }
  }
  
  // If all is well, return true
  return true;
}

bool CheckWorkspacesMatch::checkAxes(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
{
  const int numAxes = ws1->axes();
  
  if ( numAxes != ws2->axes() )
  {
    result = "Different numbers of axes";
    return false;
  }
  
  for ( int i = 0; i < numAxes; ++i )
  {
    const Axis * const ax1 = ws1->getAxis(i);
    const Axis * const ax2 = ws2->getAxis(i);
    
    if ( ax1->isNumeric() != ax2->isNumeric() )
    {
      result = "Axis type mismatch";
      return false;
    }

    if ( ax1->title() != ax2->title() )
    {
      result = "Axis title mismatch";
      return false;
    }
    
    Unit_const_sptr ax1_unit = ax1->unit();
    Unit_const_sptr ax2_unit = ax2->unit();
    
    if ( (ax1_unit == NULL && ax2_unit != NULL) || (ax1_unit != NULL && ax2_unit == NULL) 
         || ( ax1_unit && ax1_unit->unitID() != ax2_unit->unitID() ) )
    {
      result = "Axis unit mismatch";
      return false;
    }
    
    // Use Axis's equality operator to check length and values
    if ( ! ax1->operator==(*ax2) )
    {
      result = "Axis values mismatch";
      return false;
    }
  }
  
  
  if ( ws1->YUnit() != ws2->YUnit() )
  {
    result = "YUnit mismatch";
    return false;
  }
  
  // Everything's OK with the axes
  return true;
}

bool CheckWorkspacesMatch::checkSpectraMap(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
{
  // Use the SpectraDetectorMap::operator== to check the maps
//  if ( ! ws1->spectraMap().operator==(ws2->spectraMap()) )
  if ( ws1->spectraMap() != ws2->spectraMap() )
  {
    result = "SpectraDetectorMap mismatch";
    return false;
  }
  
  // Everything's OK if we get to here
  return true;
}

bool CheckWorkspacesMatch::checkInstrument(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
{
  // First check the name matches
  if ( ws1->getInstrument()->getName() != ws2->getInstrument()->getName() )
  {
    g_log.debug() << "Instrument names: WS1 = " << ws1->getInstrument()->getName() <<
                                      " WS2 = " << ws2->getInstrument()->getName() << "\n";
    result = "Instrument name mismatch";
    return false;
  }
  
  const Geometry::ParameterMap& ws1_parmap = ws1->instrumentParameters();
  const Geometry::ParameterMap& ws2_parmap = ws2->instrumentParameters();
    
  if ( ws1_parmap.asString() != ws2_parmap.asString() )
  {
    g_log.debug() << "Parameter maps...\n";
    g_log.debug() << "WS1: " << ws1_parmap.asString() << "\n";
    g_log.debug() << "WS2: " << ws2_parmap.asString() << "\n";
    result = "Instrument ParameterMap mismatch";
    return false;
  }
  
  // All OK if we're here
  return true;
}

bool CheckWorkspacesMatch::checkMasking(API::MatrixWorkspace_const_sptr ws1, API::MatrixWorkspace_const_sptr ws2)
{
  const int numHists = ws1->getNumberHistograms();
  
  for ( int i = 0; i < numHists; ++i)
  {
    const bool ws1_masks = ws1->hasMaskedBins(i);
    if ( ws1_masks != ws2->hasMaskedBins(i) )
    {
      g_log.debug() << "Only one workspace has masked bins for spectrum " << i << "\n";
      result = "Masking mismatch";
      return false;
    }
    
    // If there are masked bins, check that they match
    if ( ws1_masks && ws1->maskedBins(i) != ws2->maskedBins(i) )
    {
      g_log.debug() << "Mask lists for spectrum " << i << " do not match\n";
      result = "Masking mismatch";
      return false;     
    }
  }
  
  // All OK if here
  return true;
}


} // namespace Algorithms
} // namespace Mantid

