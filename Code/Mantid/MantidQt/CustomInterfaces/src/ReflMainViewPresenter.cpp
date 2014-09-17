#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidAPI/AlgorithmManager.h"
using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {

    const int ReflMainViewPresenter::COL_RUNS(0);
    const int ReflMainViewPresenter::COL_ANGLE(1);
    const int ReflMainViewPresenter::COL_TRANSMISSION(2);
    const int ReflMainViewPresenter::COL_QMIN(3);
    const int ReflMainViewPresenter::COL_QMAX(4);
    const int ReflMainViewPresenter::COL_DQQ(5);
    const int ReflMainViewPresenter::COL_SCALE(6);
    const int ReflMainViewPresenter::COL_GROUP(7);
    const int ReflMainViewPresenter::COL_OPTIONS(8);

    ReflMainViewPresenter::ReflMainViewPresenter(ReflMainView* view): m_view(view)
    {
    }

    ReflMainViewPresenter::ReflMainViewPresenter(ITableWorkspace_sptr model, ReflMainView* view): m_model(model), m_view(view)
    {
    }

    ReflMainViewPresenter::~ReflMainViewPresenter()
    {
    }

    /**
    Process selected rows
    */
    void ReflMainViewPresenter::process()
    {
      if(m_model->rowCount() == 0)
      {
        m_view->giveUserWarning("Cannot process an empty Table","Warning");
        return;
      }

      std::vector<size_t> rows = m_view->getSelectedRowIndexes();
      if(rows.size() == 0)
      {
        //Does the user want to abort?
        if(!m_view->askUserYesNo("This will process all rows in the table. Continue?","Process all rows?"))
          return;

        //They want to process all rows, so populate rows with every index in the model
        for(size_t idx = 0; idx < m_model->rowCount(); ++idx)
          rows.push_back(idx);
      }

      try
      {
        //TODO: Handle groups and stitch them together accordingly
        for(auto it = rows.begin(); it != rows.end(); ++it)
          processRow(*it);
      }
      catch(std::exception& ex)
      {
        m_view->giveUserCritical("Error encountered while processing: \n" + std::string(ex.what()),"Error");
      }
    }

    /**
    Process a specific Row
    @param rowNo : The row in the model to process
    */
    void ReflMainViewPresenter::processRow(size_t rowNo)
    {
      const std::string         run = m_model->String(rowNo, COL_RUNS);
      const std::string    transStr = m_model->String(rowNo, COL_TRANSMISSION);
      const std::string transWSName = makeTransWSName(transStr);

      double   dqq = 0;
      double theta = 0;
      double  qmin = 0;
      double  qmax = 0;

      const bool   dqqGiven = !m_model->String(rowNo, COL_DQQ  ).empty();
      const bool thetaGiven = !m_model->String(rowNo, COL_ANGLE).empty();
      const bool  qminGiven = !m_model->String(rowNo, COL_QMIN ).empty();
      const bool  qmaxGiven = !m_model->String(rowNo, COL_QMAX ).empty();

      if(dqqGiven)
        Mantid::Kernel::Strings::convert<double>(m_model->String(rowNo, COL_DQQ), dqq);

      if(thetaGiven)
        Mantid::Kernel::Strings::convert<double>(m_model->String(rowNo, COL_ANGLE), theta);

      if(qminGiven)
        Mantid::Kernel::Strings::convert<double>(m_model->String(rowNo, COL_QMIN), qmin);

      if(qmaxGiven)
        Mantid::Kernel::Strings::convert<double>(m_model->String(rowNo, COL_QMAX), qmax);

      //Load the run

      IAlgorithm_sptr algLoadRun = AlgorithmManager::Instance().create("Load");
      algLoadRun->initialize();
      algLoadRun->setChild(true);
      algLoadRun->setProperty("Filename", run);
      algLoadRun->setProperty("OutputWorkspace", run + "_TOF");
      algLoadRun->execute();

      if(!algLoadRun->isExecuted())
        throw std::runtime_error("Could not open run: " + run);

      Workspace_sptr runWS = algLoadRun->getProperty("OutputWorkspace");

      //If the transmission workspace already exists, re-use it.
      MatrixWorkspace_sptr transWS;
      if(AnalysisDataService::Instance().doesExist(transWSName))
        transWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(transWSName);
      else
        transWS = makeTransWS(transStr);

      IAlgorithm_sptr algReflOne = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
      algReflOne->initialize();
      algReflOne->setChild(true);
      algReflOne->setProperty("InputWorkspace", runWS);
      algReflOne->setProperty("FirstTransmissionRun", transWS);
      algReflOne->setProperty("OutputWorkspace", run + "_IvsQ");
      algReflOne->setProperty("OutputWorkspaceWaveLength", run + "_IvsLam");
      algReflOne->setProperty("ThetaIn", theta);
      algReflOne->execute();

      if(!algReflOne->isExecuted())
        throw std::runtime_error("Failed to run ReflectometryReductionOneAuto.");

      MatrixWorkspace_sptr runWSQ = algReflOne->getProperty("OutputWorkspace");
      MatrixWorkspace_sptr runWSLam = algReflOne->getProperty("OutputWorkspaceWaveLength");

      std::vector<double> built_params;
      built_params.push_back(qmin);
      built_params.push_back(-dqq);
      built_params.push_back(qmax);

      IAlgorithm_sptr algRebinQ = AlgorithmManager::Instance().create("Rebin");
      algRebinQ->initialize();
      algRebinQ->setChild(true);
      algRebinQ->setProperty("InputWorkspace", runWSQ);
      algRebinQ->setProperty("Params", built_params);
      algRebinQ->setProperty("OutputWorkspace", run + "_IvsQ_binned");
      algRebinQ->execute();

      IAlgorithm_sptr algRebinLam = AlgorithmManager::Instance().create("Rebin");
      algRebinLam->initialize();
      algRebinLam->setChild(true);
      algRebinLam->setProperty("InputWorkspace", runWSLam);
      algRebinLam->setProperty("Params", built_params);
      algRebinLam->setProperty("OutputWorkspace", run + "_IvsLam_binned");
      algRebinLam->execute();

      MatrixWorkspace_sptr runWSQBin = algRebinQ->getProperty("OutputWorkspace");
      MatrixWorkspace_sptr runWSLamBin = algRebinLam->getProperty("OutputWorkspace");

      //Finally, place the resulting workspaces into the ADS.
      AnalysisDataService::Instance().addOrReplace(run + "_TOF", runWS);

      AnalysisDataService::Instance().addOrReplace(run + "_IvsQ", runWSQ);
      AnalysisDataService::Instance().addOrReplace(run + "_IvsLam", runWSLam);

      AnalysisDataService::Instance().addOrReplace(run + "_IvsQ_binned", runWSQBin);
      AnalysisDataService::Instance().addOrReplace(run + "_IvsLam_binned", runWSLamBin);

      AnalysisDataService::Instance().addOrReplace(transWSName, transWS);
    }

    /**
    Converts a transmission workspace input string into its ADS name
    @param transString : the comma separated transmission run numbers to use
    @returns the ADS name the transmission run should be stored as
    */
    std::string ReflMainViewPresenter::makeTransWSName(const std::string& transString)
    {
      std::vector<std::string> transVec;
      boost::split(transVec, transString, boost::is_any_of(","));
      return "TRANS_" + transVec[0] + (transVec.size() > 1 ? "_" + transVec[1] : "");
    }

    /**
    Create a transmission workspace
    @param transString : the numbers of the transmission runs to use
    */
    MatrixWorkspace_sptr ReflMainViewPresenter::makeTransWS(const std::string& transString)
    {
      const size_t maxTransWS = 2;

      std::vector<std::string> transVec;
      std::vector<Workspace_sptr> transWSVec;

      //Take the first two run numbers
      boost::split(transVec, transString, boost::is_any_of(","));
      if(transVec.size() > maxTransWS)
        transVec.resize(maxTransWS);

      if(transVec.size() == 0)
        throw std::runtime_error("Failed to parse the transmission run list.");

      for(auto it = transVec.begin(); it != transVec.end(); ++it)
      {
        IAlgorithm_sptr algLoadTrans = AlgorithmManager::Instance().create("Load");
        algLoadTrans->initialize();
        algLoadTrans->setChild(true);
        algLoadTrans->setProperty("Filename", *it);
        algLoadTrans->setProperty("OutputWorkspace", "TRANS_" + *it);

        if(!algLoadTrans->isInitialized())
          break;

        algLoadTrans->execute();

        if(!algLoadTrans->isExecuted())
          break;

        transWSVec.push_back(algLoadTrans->getProperty("OutputWorkspace"));
      }

      if(transWSVec.size() != transVec.size())
        throw std::runtime_error("Failed to load one or more transmission runs. Check the run number and Mantid's data directories are correct.");

      //We have the runs, so we can create a TransWS
      IAlgorithm_sptr algCreateTrans = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
      algCreateTrans->initialize();
      algCreateTrans->setChild(true);
      algCreateTrans->setProperty("FirstTransmissionRun", boost::dynamic_pointer_cast<MatrixWorkspace>(transWSVec[0]));
      if(transWSVec.size() > 1)
        algCreateTrans->setProperty("SecondTransmissionRun", boost::dynamic_pointer_cast<MatrixWorkspace>(transWSVec[1]));

      algCreateTrans->setProperty("OutputWorkspace", makeTransWSName(transString));

      if(!algCreateTrans->isInitialized())
        throw std::runtime_error("Could not initialize CreateTransmissionWorkspaceAuto");

      algCreateTrans->execute();

      if(!algCreateTrans->isExecuted())
        throw std::runtime_error("CreateTransmissionWorkspaceAuto failed to execute");

      return algCreateTrans->getProperty("OutputWorkspace");
    }

    /**
    Add row(s) to the model
    */
    void ReflMainViewPresenter::addRow()
    {
      std::vector<size_t> rows = m_view->getSelectedRowIndexes();
      if (rows.size() == 0)
      {
        m_model->appendRow();
      }
      else
      {
        //as selections have to be contigous, then all that needs to be done is add
        //a number of rows at the highest index equal to the size of the returned vector
        std::sort (rows.begin(), rows.end());
        for (size_t idx = rows.size(); 0 < idx; --idx)
        {
          m_model->insertRow(rows.at(0));
        }
      }

      m_view->showTable(m_model);
    }

    /**
    Delete row(s) from the model
    */
    void ReflMainViewPresenter::deleteRow()
    {
      std::vector<size_t> rows = m_view->getSelectedRowIndexes();
      std::sort(rows.begin(), rows.end());
      for(size_t idx = rows.size(); 0 < idx; --idx)
        m_model->removeRow(rows.at(0));

      m_view->showTable(m_model);
    }

    /**
    Used by the view to tell the presenter something has changed
    */
    void ReflMainViewPresenter::notify(int flag)
    {
      switch(flag)
      {
      case ReflMainView::SaveAsFlag:    saveAs();     break;
      case ReflMainView::SaveFlag:      save();       break;
      case ReflMainView::AddRowFlag:    addRow();     break;
      case ReflMainView::DeleteRowFlag: deleteRow();  break;
      case ReflMainView::ProcessFlag:   process();    break;

      case ReflMainView::NoFlags:       return;
      }
      //Not having a 'default' case is deliberate. gcc issues a warning if there's a flag we aren't handling.
    }

    /**
    Load the model into the table
    */
    void ReflMainViewPresenter::load()
    {
      m_view->showTable(m_model);
    }
  }
}
