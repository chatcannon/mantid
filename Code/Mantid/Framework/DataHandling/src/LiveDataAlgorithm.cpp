#include "MantidDataHandling/LiveDataAlgorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/AlgorithmManager.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LiveDataAlgorithm::LiveDataAlgorithm()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LiveDataAlgorithm::~LiveDataAlgorithm()
  {
  }
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string LiveDataAlgorithm::category() const { return "DataHandling\\LiveData";}

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LiveDataAlgorithm::initProps()
  {
    // Options for the Instrument = all the listeners that are registered.
    std::vector<std::string> listeners = Mantid::API::LiveListenerFactory::Instance().getKeys();
    declareProperty(new PropertyWithValue<std::string>("Instrument","", boost::make_shared<StringListValidator>(listeners)),
        "Name of the instrument to monitor.");

    declareProperty(new PropertyWithValue<std::string>("StartTime","",Direction::Input),
        "Absolute start time, if you selected FromTime.\n"
        "Specify the date/time in UTC time, in ISO8601 format, e.g. 2010-09-14T04:20:12.95");

    declareProperty(new PropertyWithValue<std::string>("ProcessingAlgorithm","",Direction::Input),
        "Name of the algorithm that will be run to process each chunk of data.\n"
        "Optional. If blank, no processing will occur.");

    declareProperty(new PropertyWithValue<std::string>("ProcessingProperties","",Direction::Input),
        "The properties to pass to the ProcessingAlgorithm, as a single string.\n"
        "The format is propName=value;propName=value");

    declareProperty(new PropertyWithValue<std::string>("ProcessingScript","",Direction::Input),
        "Not currently supported, but reserved for future use.");

    std::vector<std::string> propOptions;
    propOptions.push_back("Add");
    propOptions.push_back("Replace");
    propOptions.push_back("Append");
    declareProperty("AccumulationMethod", "Add", boost::make_shared<StringListValidator>(propOptions),
        "Method to use for accumulating each chunk of live data.\n"
        " - Add: the processed chunk will be summed to the previous outpu (default).\n"
        " - Replace: the processed chunk will replace the previous output.\n"
        " - Append: the spectra of the chunk will be appended to the output workspace, increasing its size.");

    declareProperty(new PropertyWithValue<std::string>("PostProcessingAlgorithm","",Direction::Input),
        "Name of the algorithm that will be run to process the accumulated data.\n"
        "Optional. If blank, no post-processing will occur.");

    declareProperty(new PropertyWithValue<std::string>("PostProcessingProperties","",Direction::Input),
        "The properties to pass to the PostProcessingAlgorithm, as a single string.\n"
        "The format is propName=value;propName=value");

    declareProperty(new PropertyWithValue<std::string>("PostProcessingScript","",Direction::Input),
        "Not currently supported, but reserved for future use.");

    declareProperty(new WorkspaceProperty<Workspace>("AccumulationWorkspace","",Direction::Output,
                                                     PropertyMode::Optional, LockMode::NoLock),
        "Optional, unless performing PostProcessing:\n"
        " Give the name of the intermediate, accumulation workspace.\n"
        " This is the workspace after accumulation but before post-processing steps.");

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output,
                                                     PropertyMode::Mandatory, LockMode::NoLock),
                    "Name of the processed output workspace.");

    declareProperty(new PropertyWithValue<std::string>("LastTimeStamp","",Direction::Output),
                    "The time stamp of the last event, frame or pulse recorded.\n"
                    "Date/time is in UTC time, in ISO8601 format, e.g. 2010-09-14T04:20:12.95");
  }


  //----------------------------------------------------------------------------------------------
  /** Copy the LiveDataAlgorithm-specific properties from "other" to "this"
   *
   * @param other :: LiveDataAlgorithm-type algo.
   */
  void LiveDataAlgorithm::copyPropertyValuesFrom(const LiveDataAlgorithm & other)
  {
    std::vector<Property*> props = this->getProperties();
    for (size_t i=0; i < props.size(); i++)
    {
      Property*prop = props[i];
      this->setPropertyValue(prop->name(), other.getPropertyValue(prop->name()));
    }
  }


  //----------------------------------------------------------------------------------------------
  /// @return true if there is a post-processing step
  bool LiveDataAlgorithm::hasPostProcessing() const
  {
    return (!this->getPropertyValue("PostProcessingAlgorithm").empty()
        || !this->getPropertyValue("PostProcessingScript").empty());
  }

  //----------------------------------------------------------------------------------------------
  /** Return or create the ILiveListener for this algorithm.
   *
   * If the ILiveListener has not already been created, it creates it using
   * the properties on the algorithm. It then starts the listener
   * by calling the ILiveListener->start(StartTime) method.
   *
   * @return ILiveListener_sptr
   */
  ILiveListener_sptr LiveDataAlgorithm::getLiveListener()
  {
    if (m_listener)
      return m_listener;

    // Not stored? Need to create it
    std::string inst = this->getPropertyValue("Instrument");
    m_listener = LiveListenerFactory::Instance().create(inst);

    // Start at the given date/time
    m_listener->start( this->getStartTime() );

    return m_listener;
  }


  //----------------------------------------------------------------------------------------------
  /** Directly set the LiveListener for this algorithm.
   *
   * @param listener :: ILiveListener_sptr
   */
  void LiveDataAlgorithm::setLiveListener(Mantid::API::ILiveListener_sptr listener)
  {
    m_listener = listener;
  }


  //----------------------------------------------------------------------------------------------
  /** @return the value of the StartTime property */
  Mantid::Kernel::DateAndTime LiveDataAlgorithm::getStartTime() const
  {
    std::string date = getPropertyValue("StartTime");
    if (date.empty())
      return DateAndTime();
    return DateAndTime(date);
  }

  //----------------------------------------------------------------------------------------------
  /** Using the [Post]ProcessingAlgorithm and [Post]ProcessingProperties properties,
   * create and initialize an algorithm for processing.
   *
   * @param postProcessing :: true to create the PostProcessingAlgorithm.
   *        false to create the ProcessingAlgorithm
   * @return shared pointer to the algorithm, ready for execution.
   *         Returns a NULL pointer if no algorithm was chosen.
   */
  IAlgorithm_sptr LiveDataAlgorithm::makeAlgorithm(bool postProcessing)
  {
    std::string prefix = "";
    if (postProcessing)
      prefix = "Post";

    // Get the name of the algorithm to run
    std::string algoName = this->getPropertyValue(prefix+"ProcessingAlgorithm");
    algoName = Strings::strip(algoName);

    // Get the script to run. Ignored if algo is specified
    std::string script = this->getPropertyValue(prefix+"ProcessingScript");
    script = Strings::strip(script);

    if (!algoName.empty())
    {
      // Properties to pass to algo
      std::string props = this->getPropertyValue(prefix+"ProcessingProperties");

      // Create the UNMANAGED algorithm
      IAlgorithm_sptr alg = this->createSubAlgorithm(algoName);
      // ...and pass it the properties
      alg->setProperties(props);

      // Warn if someone put both values.
      if (!script.empty())
        g_log.warning() << "Running algorithm " << algoName << " and ignoring the script code in " << prefix+"ProcessingScript" << std::endl;
      return alg;
    }
    else if (!script.empty())
    {
      // Run a snippet of python
      IAlgorithm_sptr alg = this->createSubAlgorithm("RunPythonScript");
      alg->setLogging(false);
      alg->setPropertyValue("Code", script);
      return alg;
    }
    else
      return IAlgorithm_sptr();
  }

  //----------------------------------------------------------------------------------------------
  /** Validate the properties together */
  std::map<std::string, std::string> LiveDataAlgorithm::validateInputs()
  {
    std::map<std::string, std::string> out;
    if (this->getPropertyValue("OutputWorkspace").empty())
      out["OutputWorkspace"] = "Must specify the OutputWorkspace.";

    // Validate inputs
    if (this->hasPostProcessing())
    {
      if (this->getPropertyValue("AccumulationWorkspace").empty())
        out["AccumulationWorkspace"] = "Must specify the AccumulationWorkspace parameter if using PostProcessing.";

      if (this->getPropertyValue("AccumulationWorkspace") == this->getPropertyValue("OutputWorkspace"))
        out["AccumulationWorkspace"] = "The AccumulationWorkspace must be different than the OutputWorkspace, when using PostProcessing.";
    }

    return out;
    //FIXME: enable the rest here

    /** Validate that the workspace names chosen are not in use already */
    std::string outName = this->getPropertyValue("OutputWorkspace");
    std::string accumName = this->getPropertyValue("AccumulationWorkspace");

    // Check that no other MonitorLiveData thread is running with the same settings
    auto it = AlgorithmManager::Instance().algorithms().begin();
    for (; it != AlgorithmManager::Instance().algorithms().end(); it++)
    {
      IAlgorithm_sptr alg = *it;
      if (alg->name() == "MonitorLiveData")
      {
        std::cout << "Other algorithm ID is " << alg->getAlgorithmID() << " and THIS id is " << this->getAlgorithmID() << std::endl;
      }
      // MonitorLiveData thread that is running, except THIS one.
      if (alg->name() == "MonitorLiveData" && (alg->getAlgorithmID() != this->getAlgorithmID())
          && alg->isRunning())
      {
        if (!accumName.empty() && alg->getPropertyValue("AccumulationWorkspace") == accumName)
          out["AccumulationWorkspace"] += "Another MonitorLiveData thread is running with the same AccumulationWorkspace. "
              "Please specify a different AccumulationWorkspace name.";
        if (alg->getPropertyValue("OutputWorkspace") == outName)
          out["OutputWorkspace"] += "Another MonitorLiveData thread is running with the same OutputWorkspace. "
              "Please specify a different OutputWorkspace name.";
      }
    }

    return out;
  }


  //----------------------------------------------------------------------------------------------
  /** Perform validation of the inputs.
   * This should be called before starting the listener to give fast feedback
   * to the user that they did something wrong.
   *
   * @throw std::invalid_argument if there is a problem.
   */
  void LiveDataAlgorithm::throwIfInvalidInputs()
  {
    std::map<std::string, std::string> out = this->validateInputs();
    for (auto it = out.begin(); it != out.end(); it++)
      throw std::invalid_argument(it->first + ": " + it->second);
  }

} // namespace Mantid
} // namespace DataHandling
