#ifndef MD_TEST_WORKSPACE_H
#define MD_TEST_WORKSPACE_H

#include "MDDataObjects/MDWorkspace.h"
#include "MDDataObjects/MD_FileTestDataGenerator.h"

/**  Class builds a workspace filled with test data points. It repeats the logic of an load algorithm, but separated 
     here for fast data access;
     The class intended to test use an algorithm on a huge datasets without caring about an IO operations

    @author Alex Buts, RAL ISIS
    @date 21/01/2011

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>

*/


namespace Mantid
{
namespace MDDataObjects
{

class MDTestWorkspace
{
   // auxiliary variables extracted from workspace;
    IMD_FileFormat           * pReader;
    MDDataPoints             * pMDDPoints;
    MDImage                  * pMDImg;
  // workspace itself
    MDWorkspace_sptr spMDWs;

    void createTestWorkspace(IMD_FileFormat *pReader){
    // shared pointers are related to the objects, which are referenced within another objects, and auto-pointers -- to the single 
        std::auto_ptr<Geometry::MDGeometryBasis> pBasis;
        // create emtpy geometry and initiate it with the basis generated by a test file reader;
        pBasis = std::auto_ptr<Geometry::MDGeometryBasis>(new Geometry::MDGeometryBasis());
        pReader->read_basis(*pBasis);

        // read a geometry description from an test data reader
        Geometry::MDGeometryDescription geomDescr(pBasis->getNumDims(),pBasis->getNumReciprocalDims());
        pReader->read_MDGeomDescription(geomDescr);

        // read test point description 
        //TODO: The test point description is correlated with rebinning algorithm. Decorrelate
        MDPointDescription pd = pReader->read_pointDescriptions();

        // create and initate new workspace with data above .
        spMDWs = MDWorkspace_sptr(new MDWorkspace());
        spMDWs->init(std::auto_ptr<IMD_FileFormat>(pReader),pBasis,geomDescr,pd);

         pMDImg                    = spMDWs->get_spMDImage().get();
         pMDDPoints                = spMDWs->get_spMDDPoints().get();
        // read MDImage data (the key to the datapoints location) 
         pReader->read_MDImg_data(*pMDImg);
        // and initiate the positions of the datapoinst
         pMDDPoints->init_pix_locations();

         // tries to read all MDPoints in memory; It is impossible here, so it sets the workspace loation not in memory;
         pReader->read_pix(*pMDDPoints);
     
    }
public:
    MDTestWorkspace() {
        pReader=new MD_FileTestDataGenerator("testFile");
        createTestWorkspace(pReader);
    }

    ~MDTestWorkspace(){
        pReader = NULL;
    }
    MDWorkspace_sptr get_spWS(){return spMDWs;}
};

} // end namespaces;
}
#endif