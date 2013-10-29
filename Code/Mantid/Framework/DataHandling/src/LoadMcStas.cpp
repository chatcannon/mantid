/*WIKI*

Reads a McStas Nexus file into a Mantid WorkspaceGroup with a user-supplied name. Data generated by McStas monitor components are stored in workspaces of type Workspace2D or Event. 


LoadMcStas replaces LoadMcStasEventNexus.
LoadMcStas can be used for reading McStas 2.1 histogram and evnet data. 
LoadMcStasNexus can be used for reading McStas 2.0 histogram data. 



McStas 2.1 event data are generated as follows. The McStas component monitor_nD must be called with the argument: options ="mantid square x limits=[-0.2 0.2] bins=128 y limits=[-0.2 0.2] bins=128, neutron pixel t, list all neutrons".  Number of bins and limits can be chosen freely. 

To generate McStas 2.1 event data and the corresponding IDF for Mantid run the following commands from an xterm:

* export MCSTAS_CFLAGS="-g -lm -O2 -DUSE_NEXUS -lNeXus"

* mcrun -c templateSANS.instr --format=NeXus -n0

* mcdisplay templateSANS.instr -n0 --format=Mantid

* cp templateSANS.out.xml IDF.xml

* mcrun templateSANS --format=NeXus



The new features added to McStas has been tested on the following platforms:

* Linux

* Mac - use either the Intel or gcc 4.8 compiler. Simulations using Nexus format and event data does not work using the Clang compiler.  



For more information about McStas and its general usage for simulating neutron scattering instruments and experiments visit the McStas homepage http://www.mcstas.org.

*WIKI*/


#include "MantidDataHandling/LoadMcStas.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidKernel/Unit.h"
#include <nexus/NeXusFile.hpp>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/RegisterFileLoader.h"


#include "MantidAPI/NumericAxis.h"
#include <nexus/NeXusException.hpp>
#include <boost/algorithm/string.hpp>


namespace Mantid
{
namespace DataHandling 
{
	using namespace Kernel;
	using namespace API;
	using namespace DataObjects;

	// Register the algorithm into the AlgorithmFactory
	DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMcStas);


	//----------------------------------------------------------------------------------------------
	/** Constructor
	 */
  LoadMcStas::LoadMcStas() : m_countNumWorkspaceAdded(1)
  {
  }
    
	//----------------------------------------------------------------------------------------------
	/** Destructor
	 */
	LoadMcStas::~LoadMcStas()
	{
	}
  

	//----------------------------------------------------------------------------------------------
	// Algorithm's name for identification. @see Algorithm::name
	const std::string LoadMcStas::name() const { return "LoadMcStas";};
  
	// Algorithm's version for identification. @see Algorithm::version
	int LoadMcStas::version() const { return 1;};
  
	// Algorithm's category for identification. @see Algorithm::category
	const std::string LoadMcStas::category() const { return "DataHandling";}

	//----------------------------------------------------------------------------------------------
	// Sets documentation strings for this algorithm
	void LoadMcStas::initDocs()
	{
		this->setWikiSummary("This algorithm loads a McStas NeXus file into an workspace.");
		this->setOptionalMessage("Loads a McStas NeXus file into an workspace.");
	}

	//----------------------------------------------------------------------------------------------
	/** Initialize the algorithm's properties.
	 */
	void LoadMcStas::init()
	{
		std::vector<std::string> exts;
		exts.push_back(".h5");
		exts.push_back(".nxs");
		declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts), "The name of the Nexus file to load" );
		
		declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
	}


	//----------------------------------------------------------------------------------------------
	/** Execute the algorithm.
	 */
	void LoadMcStas::exec()
	{
		std::string filename = getPropertyValue("Filename");
		g_log.debug() << "Opening file " << filename << std::endl;
        
		::NeXus::File nxFile(filename);
		auto entries = nxFile.getEntries();
		auto itend = entries.end();
		WorkspaceGroup_sptr outputGroup(new WorkspaceGroup);


		// here loop over all top level Nexus entries
		// HOWEVER IF IT IS KNOWN THAT MCSTAS NEXUS ONLY EVER HAVE ONE TOP LEVEL ENTRY
		// THIS LOOP CAN BE REMOVED
		for(auto it = entries.begin(); it != itend; ++it)
		{
			std::string name = it->first;
			std::string type = it->second;
			
			// open top entry - open data entry
			nxFile.openGroup(name, type);
			nxFile.openGroup("data", "NXdetector"); 

			auto dataEntries = nxFile.getEntries();

			std::map<std::string, std::string> eventEntries;
			std::map<std::string, std::string> histogramEntries;      

			// populate eventEntries and histogramEntries
			for(auto eit = dataEntries.begin(); eit != dataEntries.end(); ++eit)
			{
				std::string dataName = eit->first;
				std::string dataType = eit->second;
				if( dataName == "content_nxs" || dataType != "NXdata" ) continue;  // can be removed if sure no Nexus files contains "content_nxs" 
				g_log.debug() << "Opening " << dataName << "   " << dataType << std::endl;

				// open second level entry
				nxFile.openGroup(dataName, dataType);

				// Find the Neutron_ID tag from McStas event data
				// Each event detector has the nexus attribute: 
				// @long_name = data ' Intensity Position Position Neutron_ID Velocity Time_Of_Flight Monitor (Square)'
				// if Neutron_ID present we have event data

				auto nxdataEntries = nxFile.getEntries();

				for(auto nit = nxdataEntries.begin(); nit != nxdataEntries.end(); ++nit)
				{
					if(nit->second == "NXparameters") continue;
					nxFile.openData(nit->first);
					if(nxFile.hasAttr("long_name") )
					{
						std::string nameAttrValue;
						nxFile.getAttr("long_name", nameAttrValue);

						if ( nameAttrValue.find("Neutron_ID") != std::string::npos )
						{
							eventEntries[eit->first] = eit->second;
						}            
						else
						{
							histogramEntries[eit->first] = eit->second;
						}
					}
					nxFile.closeData();
				} 
				// close second entry
				nxFile.closeGroup();      
			}
			

			if ( !eventEntries.empty() )      	      
			{
				readEventData(eventEntries, outputGroup, nxFile);   
			}
      
			readHistogramData(histogramEntries, outputGroup, nxFile);
			
			// close top entery 
			nxFile.closeGroup(); // corresponds to nxFile.openGroup("data", "NXdetector"); 
			nxFile.closeGroup();
			
			setProperty("OutputWorkspace", outputGroup);
		} 
	} // LoadMcStas::exec()

  
   
	/**
	 * Return the confidence with with this algorithm can load the file
	 * @param descriptor A descriptor for the file
	 * @param descriptor A descriptor for the file
	 * @param nxFile Reads data from inside first first top entry
	 */
	void LoadMcStas::readEventData(const std::map<std::string, std::string>& eventEntries, WorkspaceGroup_sptr& outputGroup, ::NeXus::File& nxFile)
	{
		std::string filename = getPropertyValue("Filename");
		auto entries = nxFile.getEntries();
		auto itend = entries.end();

		// will assume that each top level entry contain one mcstas
		// generated IDF and any event data entries within this top level
		// entry are data collected for that instrument
		// This code for loading the instrument is for now adjusted code from
		// ExperimentalInfo. 

		// Close data folder and go back to top level. Then read and close the Instrument folder. 
		nxFile.closeGroup();

		Geometry::Instrument_sptr instrument;
      
		try
		{
			nxFile.openGroup("instrument", "NXinstrument");
			std::string instrumentXML;
			nxFile.openGroup("instrument_xml", "NXnote");
			nxFile.readData("data", instrumentXML );
			nxFile.closeGroup();
			nxFile.closeGroup();

			Geometry::InstrumentDefinitionParser parser;
			std::string instrumentName = "McStas";
			parser.initialize(filename, instrumentName, instrumentXML);
			std::string instrumentNameMangled = parser.getMangledName();

			// Check whether the instrument is already in the InstrumentDataService
			if ( InstrumentDataService::Instance().doesExist(instrumentNameMangled) )
			{
				// If it does, just use the one from the one stored there
				instrument = InstrumentDataService::Instance().retrieve(instrumentNameMangled);
			}
			else
			{
				// Really create the instrument
				instrument = parser.parseXML(NULL);
				// Add to data service for later retrieval
				InstrumentDataService::Instance().add(instrumentNameMangled, instrument);
			}
		}
		catch(...)
		{
			// Loader should not stop if there is no IDF.xml
			g_log.warning() <<"\nCould not find the instrument description in the Nexus file:" << filename << " Ignore evntdata from data file" << std::endl;
			return;
		} 


		// Finished reading Instrument. Then open new data folder again
		nxFile.openGroup("data", "NXdetector");

		// create and prepare an event workspace ready to receive the mcstas events
		EventWorkspace_sptr eventWS(new EventWorkspace());
		// initialize, where create up front number of eventlists = number of detectors
		eventWS->initialize(instrument->getNumberDetectors(),1,1);
		// Set the units
		eventWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
		eventWS->setYUnit("Counts");
		// set the instrument
		eventWS->setInstrument(instrument);
		// assign detector ID to eventlists

		std::vector<detid_t> detIDs = instrument->getDetectorIDs();
    
		for (size_t i = 0; i < instrument->getNumberDetectors(); i++)
		{
			eventWS->getEventList(i).addDetectorID(detIDs[i]);
			// spectrum number are treated as equal to detector IDs for McStas data
			eventWS->getEventList(i).setSpectrumNo(detIDs[i]);  
		}
		// the one is here for the moment for backward compatibility
		eventWS->rebuildSpectraMapping(true);
   

		bool isAnyNeutrons=false;  
		// to store shortest and longest recorded TOF
		double shortestTOF(0.0); 
		double longestTOF(0.0);

		for(auto eit = eventEntries.begin(); eit != eventEntries.end(); ++eit)
		{
			std::string dataName = eit->first;
			std::string dataType = eit->second;

			// open second level entry 
			nxFile.openGroup(dataName, dataType);
			std::vector<double> data;
			nxFile.openData("events");
			nxFile.getData(data);
			nxFile.closeData();
			nxFile.closeGroup();

			// Need to take into account that the nexus readData method reads a multi-column data entry
			// into a vector  
			// The number of data column for each neutron is here hardcoded to (p, x,  y,  n, id, t)
			// Thus  we have
			// column  0 : p 	neutron wight
			// column  1 : x 	x coordinate
			// column  2 : y 	y coordinate
			// column  3 : n 	accumulated number of neutrons
      // column  4 : id 	pixel id
			// column  5 : t 	time

			size_t numberOfDataColumn = 6;
			// The number of neutrons 
			size_t nNeutrons = data.size() / numberOfDataColumn;
			if (isAnyNeutrons==false && nNeutrons>0) isAnyNeutrons=true;

			// populate workspace with McStas events          
			const detid2index_map detIDtoWSindex_map =	eventWS->getDetectorIDToWorkspaceIndexMap(true);			
			// This one does not compile on Mac			
			//detid2index_map* detIDtoWSindex_map =	eventWS->getDetectorIDToWorkspaceIndexMap(true); 

			for (size_t in = 0; in < nNeutrons; in++)
			{   
				const int detectorID = static_cast<int>(data[4+numberOfDataColumn*in]);
				const double detector_time =  data[5+numberOfDataColumn*in] * 1.0e6;  // convert to microseconds
				if ( in == 0 )
				{
					shortestTOF = detector_time;
					longestTOF = detector_time;
				}
				else
				{
					if ( detector_time < shortestTOF )	shortestTOF = detector_time;
					if ( detector_time > longestTOF )		longestTOF = detector_time;
				}

				// This one does not compile on Mac
				//size_t workspaceIndex = (*detIDtoWSindex_map)[detectorID]; 
				const size_t workspaceIndex = detIDtoWSindex_map.find(detectorID)->second;	
 

				int64_t pulse_time = 0;
				//eventWS->getEventList(workspaceIndex) += TofEvent(detector_time,pulse_time);
				//eventWS->getEventList(workspaceIndex) += TofEvent(detector_time);
				eventWS->getEventList(workspaceIndex) += WeightedEvent(detector_time, pulse_time, data[numberOfDataColumn*in], 1.0);
			}		      
		}


		// Create a default TOF-vector for histogramming, for now just 2 bins
		// 2 bins is the standard. However for McStas simulation data it may make sense to 
		// increase this number for better initial visual effect
		Kernel::cow_ptr<MantidVec> axis;
		MantidVec& xRef = axis.access();
		xRef.resize(2,0.0);
		//if ( nNeutrons > 0)
		if(isAnyNeutrons)
		{
			xRef[0] = shortestTOF - 1; //Just to make sure the bins hold it all
			xRef[1] = longestTOF + 1;
		}
		// Set the binning axis
		eventWS->setAllX(axis);

    // ensure that specified name is given to workspace (eventWS) when added to outputGroup
    std::string nameOfGroupWS = getProperty("OutputWorkspace");
    std::string nameUserSee = std::string("EventData_") + nameOfGroupWS;
    std::string extraProperty = "Outputworkspace_dummy_" + boost::lexical_cast<std::string>(m_countNumWorkspaceAdded);
    declareProperty(new WorkspaceProperty<Workspace> (extraProperty, nameUserSee, Direction::Output));
    setProperty(extraProperty, boost::static_pointer_cast<Workspace>(eventWS));
    m_countNumWorkspaceAdded++; // need to increment to ensure extraProperty are unique
		
		outputGroup->addWorkspace(eventWS);
	}

  
	/**
	 * Return the confidence with with this algorithm can load the file
	 * @param descriptor A descriptor for the file
	 * @returns An integer specifying the confidence level. 0 indicates it will not be used
	 */
	void LoadMcStas::readHistogramData(const std::map<std::string, std::string>& histogramEntries, WorkspaceGroup_sptr& outputGroup, ::NeXus::File& nxFile)
	{	

		std::string nameAttrValueYLABEL;

		for(auto eit = histogramEntries.begin(); eit != histogramEntries.end(); ++eit)
		{
			std::string dataName = eit->first;
			std::string dataType = eit->second;

 			// open second level entry
 			nxFile.openGroup(dataName,dataType);

      // grap title to use to e.g. create workspace name
      std::string nameAttrValueTITLE;
      nxFile.getAttr("title", nameAttrValueTITLE);

			if ( nxFile.hasAttr("ylabel") )
			{
				nxFile.getAttr("ylabel", nameAttrValueYLABEL);
			}

			// Find the axis names
			auto nxdataEntries = nxFile.getEntries();
			std::string axis1Name,axis2Name;
			for(auto nit = nxdataEntries.begin(); nit != nxdataEntries.end(); ++nit)
			{
				if(nit->second == "NXparameters") continue;
				if(nit->first == "ncount") continue;
				nxFile.openData(nit->first);

				if(nxFile.hasAttr("axis") )
				{
					int axisNo(0);
					nxFile.getAttr("axis", axisNo);
					if(axisNo == 1) axis1Name = nit->first;
					else if(axisNo==2) axis2Name = nit->first;
					else throw std::invalid_argument("Unknown axis number");
				}
				nxFile.closeData();
			}


			std::vector<double> axis1Values,axis2Values;
			nxFile.readData<double>(axis1Name,axis1Values);		
			if (axis2Name.length()==0) 
			{
				axis2Name=nameAttrValueYLABEL;
				axis2Values.push_back(0.0);
			} 		
			else 
			{
				nxFile.readData<double>(axis2Name,axis2Values);
			}

			const size_t axis1Length = axis1Values.size();
			const size_t axis2Length = axis2Values.size();
			g_log.debug() << "Axis lengths=" << axis1Length << " " << axis2Length << std::endl;
   

			// Require "data" field
			std::vector<double> data;
			nxFile.readData<double>("data", data);
    
			// Optional errors field
			std::vector<double> errors;
			try
			{
				nxFile.readData<double>("errors", errors);
			}
			catch(::NeXus::Exception&)
			{
				g_log.information() << "Field " << dataName << " contains no error information." << std::endl;
			}

			// close second level entry
			nxFile.closeGroup();

			MatrixWorkspace_sptr ws = 
			WorkspaceFactory::Instance().create("Workspace2D", axis2Length, axis1Length, axis1Length);
			Axis *axis1 = ws->getAxis(0);
			axis1->title() = axis1Name;
			// Set caption
			boost::shared_ptr<Units::Label> lblUnit(new Units::Label);
			lblUnit->setLabel(axis1Name,"");
			axis1->unit() = lblUnit;
  
		  	      
			Axis *axis2 = new NumericAxis(axis2Length);
			axis2->title() = axis2Name;
			// Set caption
			lblUnit = boost::shared_ptr<Units::Label>(new Units::Label);
			lblUnit->setLabel(axis2Name,"");
			axis2->unit() = lblUnit;
  
			ws->setYUnit(axis2Name);
			ws->replaceAxis(1, axis2);
  
			for(size_t wsIndex=0; wsIndex < axis2Length; ++wsIndex)
			{
				auto &dataY = ws->dataY(wsIndex);
				auto &dataE = ws->dataE(wsIndex);
				auto &dataX = ws->dataX(wsIndex);

				for(size_t j=0; j < axis1Length; ++j)
				{
					// Data is stored in column-major order so we are translating to
					// row major for Mantid
					const size_t fileDataIndex = j*axis2Length + wsIndex;

					dataY[j] = data[fileDataIndex];
					dataX[j] = axis1Values[j];
					if(!errors.empty()) dataE[j] = errors[fileDataIndex];
				}
				axis2->setValue(wsIndex, axis2Values[wsIndex]);
			} 
			
      // set the workspace title
      ws->setTitle(nameAttrValueTITLE);	

      // use the workspace title to create the workspace name
      std::replace(nameAttrValueTITLE.begin(), nameAttrValueTITLE.end(), ' ', '_');

      // ensure that specified name is given to workspace (eventWS) when added to outputGroup
      std::string nameOfGroupWS = getProperty("OutputWorkspace");
      std::string nameUserSee = nameAttrValueTITLE + "_" + nameOfGroupWS; 
      std::string extraProperty = "Outputworkspace_dummy_" + boost::lexical_cast<std::string>(m_countNumWorkspaceAdded);
      declareProperty(new WorkspaceProperty<Workspace> (extraProperty, nameUserSee, Direction::Output));
      setProperty(extraProperty, boost::static_pointer_cast<Workspace>(ws));
      m_countNumWorkspaceAdded++; // need to increment to ensure extraProperty are unique

			// Make Mantid store the workspace in the group        
			outputGroup->addWorkspace(ws);
			
		}
		nxFile.closeGroup();

	 }  // finish 

  
  /**
   * Return the confidence with with this algorithm can load the file
   * @param descriptor A descriptor for the file
   * @returns An integer specifying the confidence level. 0 indicates it will not be used
   */
  int LoadMcStas::confidence(Kernel::NexusDescriptor & descriptor) const
  {
  	using namespace ::NeXus;
	  // We will look at the first entry and check for a
	  // simulation class that contains a name attribute with the value=mcstas
	  int confidence(0);
	  try
	  {
			::NeXus::File file = ::NeXus::File(descriptor.filename());
		  auto entries = file.getEntries();
		  if(!entries.empty())
		  {
			  auto firstIt = entries.begin();
			  file.openGroup(firstIt->first,firstIt->second);
			  file.openGroup("simulation", "NXnote");
			  std::string nameAttrValue;
        file.readData("name", nameAttrValue);
			  if(boost::iequals(nameAttrValue, "mcstas")) confidence = 98;
			  file.closeGroup();
			  file.closeGroup();
		  }
	  }
	  catch(::NeXus::Exception&)
	  {
	  }
	  return confidence;
  }

} // namespace DataHandling
} // namespace Mantid