#include "MantidQtCustomInterfaces/ReflNullMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    void ReflNullMainViewPresenter::notify(int flag)
    {
      (void)flag;
      throw std::runtime_error("Cannot notify a null presenter");
    }

    ReflNullMainViewPresenter::~ReflNullMainViewPresenter()
    {
    }
  }
}
