/*WIKI*
 TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
 *WIKI*/

#include "MantidDataHandling/LoadILLIndirect.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"

#include <boost/algorithm/string.hpp>

#include <nexus/napi.h>
#include <iostream>
#include <iomanip>      // std::setw

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM (LoadILLIndirect);



//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadILLIndirect::LoadILLIndirect() : API::IFileLoader<Kernel::NexusDescriptor>()  {

    m_numberOfChannels = 0;
    m_numberOfHistograms = 0;
    m_numberOfTubes = 0;
    m_numberOfPixelsPerTube = 0;

	m_supportedInstruments.push_back("IN16B");
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadILLIndirect::~LoadILLIndirect() {
}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string LoadILLIndirect::name() const {
	return "LoadILLIndirect";
}
;

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLIndirect::version() const {
	return 1;
}
;

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLIndirect::category() const {
	return "DataHandling";
}

 //----------------------------------------------------------------------------------------------
 /// Sets documentation strings for this algorithm
void LoadILLIndirect::initDocs() {
    this->setWikiSummary("Loads a ILL/IN16B nexus file. ");
    this->setOptionalMessage("Loads a ILL/IN16B nexus file.");
}


/**
* Return the confidence with with this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not be used
*/
int LoadILLIndirect::confidence(Kernel::NexusDescriptor & descriptor) const
{

  // fields existent only at the ILL
  if (descriptor.pathExists("/entry0/wavelength")// ILL
    && descriptor.pathExists("/entry0/experiment_identifier")// ILL
    && descriptor.pathExists("/entry0/mode")// ILL
    && descriptor.pathExists("/entry0/dataSD/dataSD")// IN16B
    && descriptor.pathExists("/entry0/instrument/Doppler/doppler_frequency")// IN16B
  ) {
      return 80;
  }
  else
  {
    return 0;
  }
}

 //----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLIndirect::init() {
    declareProperty(
      new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
      "File path of the Data file to load");

    declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name to use for the output workspace");
}



 //----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLIndirect::exec() {
    // Retrieve filename
    std::string filenameData = getPropertyValue("Filename");

    // open the root node
    NeXus::NXRoot dataRoot(filenameData);
    NXEntry firstEntry = dataRoot.openFirstEntry();

    // Load Monitor details: n. monitors x monitor contents
    std::vector< std::vector<int> > monitorsData = loadMonitors(firstEntry);

/*************
    std::cout<<"The contents of monitorsData are:"<< std::endl;
    int monitor_index = 0;
    for (auto itms = monitorsData.begin(); itms != monitorsData.end(); ++itms) {
    	std::cout << "Monitor " << monitor_index << std::endl;
		for (auto itm = itms->begin(); itm != itms->end(); ++itm)
			std::cout << ' ' << *itm;
      std::cout << std::endl;
      monitor_index++;
    }
**************/

    // Load Data details (number of tubes, channels, etc)
    loadDataDetails(firstEntry);

	std::string instrumentPath = m_loader.findInstrumentNexusPath(firstEntry);
	setInstrumentName(firstEntry, instrumentPath);

    initWorkSpace(firstEntry, monitorsData);

	g_log.debug("Building properties...");
    loadNexusEntriesIntoProperties(filenameData);

	g_log.debug("Loading data...");
    loadDataIntoTheWorkSpace(firstEntry, monitorsData);

    // load the instrument from the IDF if it exists
	g_log.debug("Loading instrument definition...");
    runLoadInstrument();

    //moveSingleDetectors(); Work in progress

    // Set the output workspace property
    setProperty("OutputWorkspace", m_localWorkspace);
}

/**
* Set member variable with the instrument name
*/
void LoadILLIndirect::setInstrumentName(const NeXus::NXEntry &firstEntry,
		const std::string &instrumentNamePath) {

	if (instrumentNamePath == "") {
		std::string message(
				"Cannot set the instrument name from the Nexus file!");
		g_log.error(message);
		throw std::runtime_error(message);
	}
	m_instrumentName = m_loader.getStringFromNexusPath(firstEntry,
			instrumentNamePath + "/name");
	boost::to_upper(m_instrumentName);// "IN16b" in file, keep it upper case.
	g_log.debug() << "Instrument name set to: " + m_instrumentName << std::endl;

}

/**
* Load Data details (number of tubes, channels, etc)
* @param entry First entry of nexus file
*/
void LoadILLIndirect::loadDataDetails(NeXus::NXEntry& entry)
{
	// read in the data
	NXData dataGroup = entry.openNXData("data");
	NXInt data = dataGroup.openIntData();

	m_numberOfTubes = static_cast<size_t>(data.dim0());
	m_numberOfPixelsPerTube = static_cast<size_t>(data.dim1());
	m_numberOfChannels = static_cast<size_t>(data.dim2());


	NXData dataSDGroup = entry.openNXData("dataSD");
	NXInt dataSD = dataSDGroup.openIntData();

	m_numberOfSimpleDetectors = static_cast<size_t>(dataSD.dim0());
}


/**
   * Load monitors data found in nexus file
   *
   * @param entry :: The Nexus entry
   *
   */
std::vector< std::vector<int> > LoadILLIndirect::loadMonitors(NeXus::NXEntry& entry){
	// read in the data
	g_log.debug("Fetching monitor data...");

	NXData dataGroup = entry.openNXData("monitor/data");
	NXInt data = dataGroup.openIntData();
	// load the counts from the file into memory
	data.load();

	// For the moment, we are aware of only one monitor entry, but we keep the generalized case of n monitors

	std::vector< std::vector<int> > monitors(1);
	std::vector<int> monitor(data(), data()+data.size());
	monitors[0].swap(monitor);
	return monitors;
}




/**
   * Creates the workspace and initialises member variables with
   * the corresponding values
   *
   * @param entry :: The Nexus entry
   * @param monitorsData :: Monitors data already loaded
   *
   */
void LoadILLIndirect::initWorkSpace(NeXus::NXEntry& /*entry*/, std::vector< std::vector<int> > monitorsData)
{

	// dim0 * m_numberOfPixelsPerTube is the total number of detectors
	m_numberOfHistograms = m_numberOfTubes * m_numberOfPixelsPerTube;

	g_log.debug() << "NumberOfTubes: " << m_numberOfTubes << std::endl;
	g_log.debug() << "NumberOfPixelsPerTube: " << m_numberOfPixelsPerTube << std::endl;
	g_log.debug() << "NumberOfChannels: " << m_numberOfChannels << std::endl;
	g_log.debug() << "NumberOfSimpleDetectors: " << m_numberOfSimpleDetectors << std::endl;
	g_log.debug() << "Monitors: " << monitorsData.size() << std::endl;
	g_log.debug() << "Monitors[0]: " << monitorsData[0].size() << std::endl;

	// Now create the output workspace

	m_localWorkspace = WorkspaceFactory::Instance().create(
			"Workspace2D",
			m_numberOfHistograms+monitorsData.size()+m_numberOfSimpleDetectors,
			m_numberOfChannels + 1,
			m_numberOfChannels);

	m_localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

	m_localWorkspace->setYUnitLabel("Counts");

}


/**
   * Load data found in nexus file
   *
   * @param entry :: The Nexus entry
   * @param monitorsData :: Monitors data already loaded
   *
   */
void LoadILLIndirect::loadDataIntoTheWorkSpace(NeXus::NXEntry& entry, std::vector< std::vector<int> > monitorsData)
{

  // read in the data
  NXData dataGroup = entry.openNXData("data");
  NXInt data = dataGroup.openIntData();
  // load the counts from the file into memory
  data.load();

  // Same for Simple Detectors
  NXData dataSDGroup = entry.openNXData("dataSD");
  NXInt dataSD = dataSDGroup.openIntData();
  // load the counts from the file into memory
  dataSD.load();


  // Assign calculated bins to first X axis
////  m_localWorkspace->dataX(0).assign(detectorTofBins.begin(), detectorTofBins.end());

  size_t spec = 0;
  size_t nb_monitors = monitorsData.size();
  size_t nb_SD_detectors = dataSD.dim0();

  Progress progress(this, 0, 1, m_numberOfTubes * m_numberOfPixelsPerTube + nb_monitors + nb_SD_detectors);

  // Assign fake values to first X axis <<to be completed>>
  for (size_t i = 0; i <= m_numberOfChannels; ++i) {
	  m_localWorkspace->dataX(0)[i] = double(i);
  }

  // First, Monitor
  for (size_t im = 0; im<nb_monitors; im++){

      if (im > 0)
      {
        m_localWorkspace->dataX(im) = m_localWorkspace->readX(0);
      }

      // Assign Y
      int* monitor_p = monitorsData[im].data();
      m_localWorkspace->dataY(im).assign(monitor_p, monitor_p + m_numberOfChannels);

	  progress.report();
  }


  // Then Tubes
  for (size_t i = 0; i < m_numberOfTubes; ++i)
  {
    for (size_t j = 0; j < m_numberOfPixelsPerTube; ++j)
    {

      // just copy the time binning axis to every spectra
      m_localWorkspace->dataX(spec+nb_monitors) = m_localWorkspace->readX(0);

      // Assign Y
      int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
      m_localWorkspace->dataY(spec+nb_monitors).assign(data_p, data_p + m_numberOfChannels);

      // Assign Error
      MantidVec& E = m_localWorkspace->dataE(spec+nb_monitors);
      std::transform(data_p, data_p + m_numberOfChannels, E.begin(),
    		LoadILLIndirect::calculateError);

      ++spec;
      progress.report();
    }
  }// for m_numberOfTubes

  // Then add Simple Detector (SD)
  for (int i = 0; i < dataSD.dim0(); ++i) {

      // just copy again the time binning axis to every spectra
      m_localWorkspace->dataX(spec+nb_monitors+i) = m_localWorkspace->readX(0);

      // Assign Y
      int* dataSD_p = &dataSD(i, 0, 0);
      m_localWorkspace->dataY(spec+nb_monitors+i).assign(dataSD_p, dataSD_p + m_numberOfChannels);

	  progress.report();
  }



}



/**
   * show attributes attaches to current nexus entry
   *
   * @param nxfileID :: The Nexus entry
   * @param indent_str :: some spaces following tree level
   *
   */
void LoadILLIndirect::dump_attributes(NXhandle nxfileID, std::string& indent_str){
	// Attributes
	NXname pName;
	int iLength, iType;
	int nbuff = 127;
	boost::shared_array<char> buff(new char[nbuff+1]);

	while(NXgetnextattr(nxfileID, pName, &iLength, &iType) != NX_EOD)
	{
		g_log.debug()<<indent_str<<'@'<<pName<<" = ";
		switch(iType)
		{
		case NX_CHAR:
			{
				if (iLength > nbuff + 1)
				{
					nbuff = iLength;
					buff.reset(new char[nbuff+1]);
				}
				int nz = iLength + 1;
				NXgetattr(nxfileID,pName,buff.get(),&nz,&iType);
				g_log.debug()<<indent_str<<buff.get()<<'\n';
				break;
			}
		case NX_INT16:
			{
				short int value;
				NXgetattr(nxfileID,pName,&value,&iLength,&iType);
				g_log.debug()<<indent_str<<value<<'\n';
				break;
			}
		case NX_INT32:
			{
				int value;
				NXgetattr(nxfileID,pName,&value,&iLength,&iType);
				g_log.debug()<<indent_str<<value<<'\n';
				break;
			}
		case NX_UINT16:
			{
				short unsigned int value;
				NXgetattr(nxfileID,pName,&value,&iLength,&iType);
				g_log.debug()<<indent_str<<value<<'\n';
				break;
			}
		}// switch
	}// while
}



/* From Ricardo Leal python show... */
/* Good old recursive walk */
void LoadILLIndirect::rl_build_properties(NXhandle nxfileID,
					API::Run& runDetails,
					std::string& parent_name,
					std::string& parent_class,
		    		int indent) {

	std::string indent_str(indent*2, ' ');// Two space by indent level

	// Link ?

	// Attributes
	//dump_attributes(nxfileID, indent_str);

	// Classes
	NXstatus stat;       ///< return status
	int datatype;        ///< NX data type if a dataset, e.g. NX_CHAR, NX_FLOAT32, see napi.h
	char nxname[NX_MAXNAMELEN],nxclass[NX_MAXNAMELEN];

	while(NXgetnextentry(nxfileID,nxname,nxclass,&datatype) != NX_EOD)
	{
		g_log.debug()<<indent_str<<parent_name<<"."<<nxname<<" ; "<<nxclass<<std::endl;

		if((stat=NXopengroup(nxfileID,nxname,nxclass))==NX_OK){

			// Go down to one level
			std::string p_nxname(nxname);//current names can be useful for next level
			std::string p_nxclass(nxclass);

			rl_build_properties(nxfileID, runDetails, p_nxname, p_nxclass, indent+1);

			NXclosegroup(nxfileID);
		}// if(NXopengroup
		else if ((stat=NXopendata (nxfileID, nxname))==NX_OK)
		{
			//dump_attributes(nxfileID, indent_str);
			g_log.debug()<<indent_str<<nxname<<" opened."<<std::endl;

			if (parent_class=="NXData") {
				g_log.debug()<<indent_str<<"skipping NXData"<<std::endl;
				/* nothing */
			} else if (parent_class=="NXMonitor") {
				g_log.debug()<<indent_str<<"skipping NXMonitor"<<std::endl;
				/* nothing */
			} else { // create a property
				int rank;
				int dims[4];
				int type;

				std::string property_name;
				// Exclude "entry0" from name for level 1 property
				if (parent_name == "entry0")
					property_name = nxname;
				else
					property_name = parent_name+"."+nxname;

				g_log.debug()<<indent_str<<"considering property "<<property_name<<std::endl;

				// Get the value
				NXgetinfo(nxfileID, &rank, dims, &type);

				//g_log.debug()<<indent_str<<"rank "<<rank<<" dims[0]"<<dims[0]<<" dims[1]"<<dims[1]<<" dims[2]"<<dims[2]<<" dims[3]"<<dims[3]<<" type "<<type<<std::endl;

				// Note, we choose to ignore "multidim properties
				if (rank!=1) {
					g_log.debug()<<indent_str<<"ignored multi dimension data on "<<property_name<<std::endl;
				} else {
					void *dataBuffer;
					NXmalloc (&dataBuffer, rank, dims, type);
					if (NXgetdata(nxfileID, dataBuffer) != NX_OK) {
						NXfree(&dataBuffer);
						throw std::runtime_error("Cannot read data from NeXus file");
					}

					if (type==NX_CHAR) {
						runDetails.addProperty(property_name, std::string((const char *)dataBuffer));
					} else if ((type==NX_FLOAT32)
								||(type==NX_FLOAT64)
								||(type==NX_INT16)
								||(type==NX_INT32)
								||(type==NX_UINT16)
								) {

						// Look for "units"
						NXstatus units_status;
						char units_sbuf[NX_MAXNAMELEN];
						int units_len=NX_MAXNAMELEN;
						int units_type=NX_CHAR;

						units_status=NXgetattr(nxfileID,const_cast<char*>("units"),(void *)units_sbuf,&units_len,&units_type);
						if(units_status!=NX_ERROR)
						{
							g_log.debug()<<indent_str<<"[ "<<property_name<<" has unit "<<units_sbuf<<" ]"<<std::endl;
						}


						if ((type==NX_FLOAT32)||(type==NX_FLOAT64)) {
							// Mantid numerical properties are double only.
							double property_double_value=0.0;
							if (type==NX_FLOAT32) {
								property_double_value = *((float*)dataBuffer);
							} else if (type==NX_FLOAT64) {
								property_double_value = *((double*)dataBuffer);
							}

							if(units_status!=NX_ERROR)
								runDetails.addProperty(property_name, property_double_value, std::string(units_sbuf));
							else
								runDetails.addProperty(property_name, property_double_value);

						} else {
							// int case
							int property_int_value=0;
							if (type==NX_INT16) {
								property_int_value = *((short int*)dataBuffer);
							} else if (type==NX_INT32) {
								property_int_value = *((int*)dataBuffer);
							}else if (type==NX_UINT16) {
								property_int_value = *((short unsigned int*)dataBuffer);
							}

							if(units_status!=NX_ERROR)
								runDetails.addProperty(property_name, property_int_value, std::string(units_sbuf));
							else
								runDetails.addProperty(property_name, property_int_value);

						}// if (type==...



					} else {
						g_log.debug()<<indent_str<<"unexpected data on "<<property_name<<std::endl;
					}

					NXfree(&dataBuffer);
				}
			}

			NXclosedata(nxfileID);
		} else {
			g_log.debug()<<indent_str<<"unexpected status ("<<stat<<") on "<<nxname<<std::endl;
		}

	}// while NXgetnextentry


}// rl_build_properties





void LoadILLIndirect::loadNexusEntriesIntoProperties(std::string nexusfilename) {

    API::Run & runDetails = m_localWorkspace->mutableRun();

    // Open NeXus file
    NXhandle nxfileID;
    NXstatus stat=NXopen(nexusfilename.c_str(), NXACC_READ, &nxfileID);
    if(stat==NX_ERROR)
    {
    	g_log.debug() << "convertNexusToProperties: Error loading " << nexusfilename;
        throw Kernel::Exception::FileError("Unable to open File:" , nexusfilename);
    }
    rl_build_properties(nxfileID, runDetails, nexusfilename, nexusfilename, 0);

    stat=NXclose(&nxfileID);
}


/**
   * Run the Child Algorithm LoadInstrument.
   */
void LoadILLIndirect::runLoadInstrument() {

	IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");

	// Now execute the Child Algorithm. Catch and log any error, but don't stop.
	try {

		// TODO: depending on the m_numberOfPixelsPerTube we might need to load a different IDF

		loadInst->setPropertyValue("InstrumentName", m_instrumentName);
		loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_localWorkspace);
		loadInst->execute();

	} catch (...) {
		g_log.information("Cannot load the instrument definition.");
	}
}


void LoadILLIndirect::moveComponent(const std::string &componentName, double twoTheta, double offSet) {

	try {

		Geometry::Instrument_const_sptr instrument = m_localWorkspace->getInstrument();
		Geometry::IComponent_const_sptr component = instrument->getComponentByName(componentName);

		double r, theta, phi, newTheta, newR;
		V3D oldPos = component->getPos();
		oldPos.getSpherical(r, theta, phi);

		newTheta = twoTheta;
		newR = offSet;

		V3D newPos;
		newPos.spherical(newR, newTheta, phi);

		//g_log.debug() << tube->getName() << " : t = " << theta << " ==> t = " << newTheta << "\n";
		Geometry::ParameterMap& pmap = m_localWorkspace->instrumentParameters();
		Geometry::ComponentHelper::moveComponent(*component, pmap, newPos, Geometry::ComponentHelper::Absolute);


	} catch (Mantid::Kernel::Exception::NotFoundError&) {
		throw std::runtime_error(
				"Error when trying to move the "  + componentName +  " : NotFoundError");
	} catch (std::runtime_error &) {
		throw std::runtime_error(
				"Error when trying to move the "  + componentName +  " : runtime_error");
	}

}

void LoadILLIndirect::moveSingleDetectors(){


	std::string prefix("single_tube_");

	for (int i=1; i<=8; i++){
		std::string componentName = prefix + boost::lexical_cast<std::string>(i);

		moveComponent(componentName, i*20.0, 2.0+i/10.0);

	}


}


} // namespace DataHandling
} // namespace Mantid
