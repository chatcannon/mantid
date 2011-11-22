#ifndef MANTID_MD_CONVERT2_QxyzDE_TEST_H_
#define MANTID_MD_CONVERT2_QxyzDE_TEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/ConvertToQ3DdE.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;

class ConvertToQ3DdETest : public CxxTest::TestSuite
{
 std::auto_ptr<ConvertToQ3DdE> pAlg;
public:
static ConvertToQ3DdETest *createSuite() { return new ConvertToQ3DdETest(); }
static void destroySuite(ConvertToQ3DdETest * suite) { delete suite; }    

void testInit(){

    TS_ASSERT_THROWS_NOTHING( pAlg->initialize() )
    TS_ASSERT( pAlg->isInitialized() )

    TSM_ASSERT_EQUALS("algortithm should have 6 propeties",6,(size_t)(pAlg->getProperties().size()));
}

void testExecThrow(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::CreateGroupedWorkspace2DWithRingsAndBoxes();

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS("the inital ws is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()),std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));

}



void testWithExistingLatticeTrowsLowEnergy(){
    // create model processed workpsace with 10x10 cylindrical detectors, 10 energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "2."));

    pAlg->execute();
    TSM_ASSERT("Should be not-successful as input energy was lower then obtained",!pAlg->isExecuted());

}
void testExecFailsOnNewWorkspaceNoLimits(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));

    pAlg->execute();
    TSM_ASSERT("Should fail as no  min-max limits were specied ",!pAlg->isExecuted());

}
void testExecFailsOnNewWorkspaceNoMaxLimits(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMin", "-50.,-50.,-50,-2"));

    pAlg->execute();
    TSM_ASSERT("Should fail as no max limits were specied ",!pAlg->isExecuted());

}
void testExecFailsLimits_MinGeMax(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMin", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMax", " 50., 50.,-50,-2"));

    pAlg->execute();
    TSM_ASSERT("Should fail as no max limits were specied ",!pAlg->isExecuted());

}
void testExecFine(){
    // create model processed workpsace with 10x10 cylindrical detectors, 10 energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMin", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMax", " 50., 50., 50, 20"));

    pAlg->execute();
    TSM_ASSERT("Should be successful ",pAlg->isExecuted());

}
void testExecAndAdd(){
    // create model processed workpsace with 10x10 cylindrical detectors, 10 energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

  // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().getGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);
 //  

     AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMin", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMax", " 50., 50., 50, 20"));


    pAlg->execute();
    TSM_ASSERT("Should be successful ",pAlg->isExecuted());
    // check if the algorith used correct energy
    TSM_ASSERT_EQUALS("The energy, used by the algorithm has not been reset from the workspace", "13",std::string(pAlg->getProperty("EnergyInput")));

}
// COMPARISON WITH HORACE:  --->
void testTransfMat1()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,2,3, 90., 90., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);

    std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,1,0));
     Kernel::Matrix<double> unit = Kernel::Matrix<double>(3,3, true);
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(unit.equals(rez,1.e-4));
}
void testTransfMat2()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,2,3, 75., 45., 35.);
     ws2D->mutableSample().setOrientedLattice(latt);

    std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,1,0));
     Kernel::Matrix<double> unit = Kernel::Matrix<double>(3,3, true);
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(unit.equals(rez,1.e-4));
}
void testTransfMat3()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,2,3, 75., 45., 35.);
     ws2D->mutableSample().setOrientedLattice(latt);

    std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,-1,0));
     Kernel::Matrix<double> unit = Kernel::Matrix<double>(3,3, true);
     unit[1][1]=-1;
     unit[2][2]=-1;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(unit.equals(rez,1.e-4));
}
void testTransfMat4()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,1,3, 90., 90., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);
     ws2D->mutableRun().getGoniometer().setRotationAngle(0,0);
     ws2D->mutableRun().getGoniometer().setRotationAngle(1,0);
     ws2D->mutableRun().getGoniometer().setRotationAngle(2,0);

    std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,1,0),Kernel::V3D(1,-1,0));
     Kernel::Matrix<double> sample = Kernel::Matrix<double>(3,3, true);
     sample[0][0]= sqrt(2.)/2 ;
     sample[0][1]= sqrt(2.)/2 ;
     sample[1][0]= sqrt(2.)/2 ;
     sample[1][1]=-sqrt(2.)/2 ;
     sample[2][2]= -1 ;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(sample.equals(rez,1.e-4));
}
void testTransfMat5()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,2,3, 75., 45., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);
     ws2D->mutableRun().getGoniometer().setRotationAngle(0,0);
     ws2D->mutableRun().getGoniometer().setRotationAngle(1,0);
     ws2D->mutableRun().getGoniometer().setRotationAngle(2,0);

    std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,1,0),Kernel::V3D(1,-1,0));
     Kernel::Matrix<double> sample = Kernel::Matrix<double>(3,3, true);
     //aa=[0.9521 0.3058  0.0000;  0.3058   -0.9521    0.0000;   0         0   -1.000];
     sample[0][0]= 0.9521 ;
     sample[0][1]= 0.3058 ;
     sample[1][0]= 0.3058 ;
     sample[1][1]=-0.9521 ;
     sample[2][2]= -1 ;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(sample.equals(rez,1.e-4));
}
void testTransf_PSI_DPSI()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,1,1, 90., 90., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);
     ws2D->mutableRun().getGoniometer().setRotationAngle(0,0); 
     ws2D->mutableRun().getGoniometer().setRotationAngle(1,-20); // Psi, dPsi
     ws2D->mutableRun().getGoniometer().setRotationAngle(2,0);

    std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,1,0));
     Kernel::Matrix<double> sample = Kernel::Matrix<double>(3,3, true);   
     sample[0][0]= 0.9397 ;
     sample[0][1]= 0.3420 ;
     sample[1][0]=-0.3420 ;
     sample[1][1]= 0.9397 ;
     sample[2][2]=  1 ;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(sample.equals(rez,1.e-4));
}
void testTransf_GL()
{
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(16,10,true);
     OrientedLattice * latt = new OrientedLattice(1,1,1, 90., 90., 90.);
     ws2D->mutableSample().setOrientedLattice(latt);
     ws2D->mutableRun().getGoniometer().setRotationAngle(0,20);  //gl
     ws2D->mutableRun().getGoniometer().setRotationAngle(1,0);
     ws2D->mutableRun().getGoniometer().setRotationAngle(2,0);

    std::vector<double> rot=pAlg->get_transf_matrix(ws2D,Kernel::V3D(1,0,0),Kernel::V3D(0,1,0));
     Kernel::Matrix<double> sample = Kernel::Matrix<double>(3,3, true);
     
     sample[0][0]= 0.9397 ;
     sample[0][2]= 0.3420 ;
     sample[2][0]=-0.3420 ;
     sample[2][2]= 0.9397 ;
     sample[1][1]=  1 ;
     Kernel::Matrix<double> rez(rot);
     TS_ASSERT(sample.equals(rez,1.e-4));
}
// check the results;
void t__tResult(){
     std::vector<double> L2(3,10),polar(3,0),azim(3,0);
     polar[1]=1;
     polar[2]=2;
     azim[0]=-1;
     azim[2]= 1;

     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedInelasticWS(L2,polar,azim,3,-1,2,10);

     ws2D->mutableRun().getGoniometer().setRotationAngle(0,0);  //gl
     ws2D->mutableRun().getGoniometer().setRotationAngle(1,0);
     ws2D->mutableRun().getGoniometer().setRotationAngle(2,0);
  
     AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMin", "-10.,-10.,-10,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QdEValuesMax", " 10., 10., 10, 8"))
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("UsePreprocessedDetectors","0"));

    pAlg->execute();
    TSM_ASSERT("Should be successful ",pAlg->isExecuted());

    Mantid::API::Workspace_sptr wsOut =  AnalysisDataService::Instance().retrieve("EnergyTransfer4DWS");
    TSM_ASSERT("Can not retrieve resulting workspace from the analysis data service ",wsOut);
}


// COMPARISON WITH HORACE: END  <---



ConvertToQ3DdETest(){
    pAlg = std::auto_ptr<ConvertToQ3DdE>(new ConvertToQ3DdE());
}

};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

