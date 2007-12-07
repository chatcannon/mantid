/*    
Copyright &copy; 2007 ???RAL???

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

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveCSV.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h" 

#include <fstream>  // used to get ofstream
#include <iomanip>  // setw() used below

DECLARE_NAMESPACED_ALGORITHM(Mantid::DataHandling, SaveCSV)

/* @class SaveCSV 

@author Anders J. Markvardsen, ISIS, RAL
@date 15/10/2007
*/

namespace Mantid
{
  namespace DataHandling
  {

    using namespace Kernel;
    using DataObjects::Workspace1D;
    using DataObjects::Workspace2D;

    Logger& SaveCSV::g_log = Logger::get("SaveCSV");


    /// Empty default constructor
    SaveCSV::SaveCSV() {}


    /** Initialisation method. Does nothing at present.
    * 
    */
    void SaveCSV::init()
    {	
      declareProperty("Filename","",new Kernel::MandatoryValidator);
      declareProperty("Separator",",");
      declareProperty("LineSeparator","\n");
    }


    /** Executes the algorithm. Retrieve the Filename, separator and Lineseparator
    *  properties and save workspace to Filename. 
    * 
    *  @throw NotImplementedError Thrown if workspace to save is not a 1D or 2D workspace
    *  @throw Mantid::Kernel::Exception::FileError  Thrown if errors with file opening and existence 
    */
    void SaveCSV::exec()
    {
      // Gets the name of the file to save the workspace to; and the
      // separator and Lineseparator properties if they are provided by the user.

      // Retrieve the filename from the properties
      m_filename = getPropertyValue("Filename");

      // Get the values of the optional parameters 
      m_separator = getPropertyValue("Separator");
      m_lineSeparator = getPropertyValue("LineSeparator");


      // prepare to save to file

      std::ofstream outCSV_File(m_filename.c_str());  

      if (!outCSV_File)
      {
        g_log.error("Failed to open file:" + m_filename);
        throw Exception::FileError("Failed to open file:" , m_filename);	  
      }


      // get workspace ID string. Used to differentiate between
      // workspace1D and workspace2D in the if statement below

      const std::string workspaceID = m_inputWorkspace->id();


      // seperating out code depending on the workspace ID


      if ( workspaceID == "Workspace1D" )
      {

        const Workspace1D *localworkspace = dynamic_cast<Workspace1D*>(m_inputWorkspace);


        // Get info from 1D workspace

        const std::vector<double>& xValue = localworkspace->dataX();
        const std::vector<double>& yValue = localworkspace->dataY();
        const std::vector<double>& eValue = localworkspace->dataE();


        // write to file

        for (int i = 0; i < (int)xValue.size(); i++)
        {
          outCSV_File << std::setw(15) << xValue[i] << m_separator << std::setw(15) << yValue[i] 
          << m_separator << std::setw(15) << eValue[i] << m_lineSeparator;
        }    
      }
      else if ( workspaceID == "Workspace2D" )
      {

        const Workspace2D *localworkspace = dynamic_cast<Workspace2D*>(m_inputWorkspace);


        // Get info from 2D workspace

        const int numberOfHist = localworkspace->getHistogramNumber();


        // Add first x-axis line to output file

        {
          const std::vector<double>& xValue = localworkspace->getX(0);

          outCSV_File << "A";

          for (int j = 0; j < (int)xValue.size(); j++)
          {
            outCSV_File << std::setw(15) << xValue[j] << m_separator;
          }

          outCSV_File << m_lineSeparator;	
        }


        for (int i = 0; i < numberOfHist; i++)
        {
          // check if x-axis has changed. If yes print out new x-axis line

          if (i > 0)
          {
            const std::vector<double>& xValue = localworkspace->getX(i);
            const std::vector<double>& xValuePrevious = localworkspace->getX(i-1);

            if ( xValue != xValuePrevious )
            {
              outCSV_File << "A";

              for (int j = 0; j < (int)xValue.size(); j++)
              {
                outCSV_File << std::setw(15) << xValue[j] << m_separator;
              }

              outCSV_File << m_lineSeparator;
            }
          }


          // add y-axis line for histogram (detector) i

          const std::vector<double>& yValue = localworkspace->getY(i);

          outCSV_File << i;

          for (int j = 0; j < (int)yValue.size(); j++)
          {
            outCSV_File << std::setw(15) << yValue[j] << m_separator;
          }  

          outCSV_File << m_lineSeparator;
        }						
        // print out errors

        outCSV_File << "\nERRORS\n";			

        for (int i = 0; i < numberOfHist; i++)
        {
          const std::vector<double>& eValue = localworkspace->getE(i);

          outCSV_File << i;

          for (int j = 0; j < (int)eValue.size(); j++)
          {
            outCSV_File << std::setw(15) << eValue[j] << m_separator;
          }  				
          outCSV_File << m_lineSeparator;
        }			
      }
      else
      {
        outCSV_File.close();  // and should probably delete file from disk as well			
        throw Exception::NotImplementedError("SaveCSV currently only works for 1D and 2D workspaces.");
      }		
      outCSV_File.close();
      //only gets here if everything happened normally
      return;
    }  
    /** Finalisation method. Does nothing at present.
    *
    */  
    void SaveCSV::final()
    {
    }  

  } // namespace DataHandling
} // namespace Mantid
