#ifndef MANTID_CUSTOMINTERFACES_REFLBLANKMAINVIEWPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLBLANKMAINVIEWPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/make_shared.hpp>
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtCustomInterfaces/ReflBlankMainViewPresenter.h"

#include "ReflMainViewMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

class ReflBlankMainViewPresenterTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflBlankMainViewPresenterTest *createSuite() { return new ReflBlankMainViewPresenterTest(); }
  static void destroySuite( ReflBlankMainViewPresenterTest *suite ) { delete suite; }

  void testEditSave()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);

    //Set up some data
    mockView.addDataForTest();

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "save" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the workspace was saved correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "13460");
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 3);

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testSaveAs()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "save as" but cancels when choosing a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return(""));
    presenter.notify(SaveAsFlag);

    //The user hits "save as" and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveAsFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check the workspace was saved
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testSaveProcess()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "save as" but cancels when choosing a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return(""));
    presenter.notify(SaveAsFlag);

    //The user hits "save as" and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveAsFlag);

    //The user hits "save" and is not asked for a name
    EXPECT_CALL(mockView, askUserString(_,_,_)).Times(0);
    presenter.notify(SaveFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check the workspace was saved
    TS_ASSERT(AnalysisDataService::Instance().doesExist("Workspace"));

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAddRow()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);

    //Set up some data
    mockView.addDataForTest();

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "add row" twice with no rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(2).WillRepeatedly(Return(std::vector<size_t>()));
    presenter.notify(AddRowFlag);
    presenter.notify(AddRowFlag);

    //The user hits "save" and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the workspace was saved correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 0);

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAddRowSpecify()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(1);

    //Set up some data
    mockView.addDataForTest();

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "add row twice" with the second row selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(2).WillRepeatedly(Return(rowlist));
    presenter.notify(AddRowFlag);
    presenter.notify(AddRowFlag);

    //The user hits "save" and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the workspace was saved correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 6);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "13470");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testAddRowSpecifyPlural()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(1);
    rowlist.push_back(2);
    rowlist.push_back(3);

    //Set up some data
    mockView.addDataForTest();

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "add row" once with the second, third, and fourth rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(AddRowFlag);

    //The user hits "save" and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the workspace was saved correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 7);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(2, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(3, RunCol), "");
    TS_ASSERT_EQUALS(ws->String(4, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->String(5, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->String(6, RunCol), "13470");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(4, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(5, GroupCol), 1);
    TS_ASSERT_EQUALS(ws->Int(6, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testDeleteRowNone()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);

    //Set up some data
    mockView.addDataForTest();

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The usert hits "delete row" with no rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(std::vector<size_t>()));
    presenter.notify(DeleteRowFlag);

    //The user hits "save" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the workspace was saved correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13462");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 3);

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testDeleteRowSingle()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(1);

    //Set up some data
    mockView.addDataForTest();

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "delete" with the second row selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(DeleteRowFlag);

    //The user hits "save" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the workspace was saved correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 3);
    TS_ASSERT_EQUALS(ws->String(1, RunCol), "13469");
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testDeleteRowPlural()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(0);
    rowlist.push_back(1);
    rowlist.push_back(2);

    //Set up some data
    mockView.addDataForTest();

    //We should not receive any errors
    EXPECT_CALL(mockView,  giveUserCritical(_,_)).Times(0);

    //The user hits "delete" with the first three rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(DeleteRowFlag);

    //The user hits "save" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveFlag);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Check that the workspace was saved correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->String(0, RunCol), "13470");
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 1);

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }

  void testGroupRows()
  {
    MockView mockView;
    ReflBlankMainViewPresenter presenter(&mockView);
    std::vector<size_t> rowlist;
    rowlist.push_back(1);
    rowlist.push_back(2);

    //Set up some data
    mockView.addDataForTest();

    //We should not receive any errors
    EXPECT_CALL(mockView, giveUserCritical(_,_)).Times(0);

    //The user hits "group rows" with the middle two rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(GroupRowsFlag);

    //The user hits "save" and and enters "Workspace" for a name
    EXPECT_CALL(mockView, askUserString(_,_,"Workspace")).Times(1).WillRepeatedly(Return("Workspace"));
    presenter.notify(SaveFlag);

    //Check that the workspace was saved correctly
    ITableWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 3);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 1);

    //Let's do it again, but with different rows
    rowlist.clear();
    rowlist.push_back(0);
    rowlist.push_back(1);

    //The user hits "group rows" with the first two rows selected
    EXPECT_CALL(mockView, getSelectedRowIndexes()).Times(1).WillRepeatedly(Return(rowlist));
    presenter.notify(GroupRowsFlag);

    //The user hits save and is not asked for a workspace name
    presenter.notify(SaveFlag);

    //Check that the workspace was save correctly
    ws = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("Workspace");
    TS_ASSERT_EQUALS(ws->rowCount(), 4);
    TS_ASSERT_EQUALS(ws->Int(0, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(1, GroupCol), 2);
    TS_ASSERT_EQUALS(ws->Int(2, GroupCol), 0);
    TS_ASSERT_EQUALS(ws->Int(3, GroupCol), 1);

    //Check calls were made as expected
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockView));

    //Tidy up
    AnalysisDataService::Instance().remove("Workspace");
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLBLANKMAINVIEWPRESENTERTEST_H_ */
