#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ISpectrum::ISpectrum()
  : m_specNo(0),
    detectorIDs(), refX(), refDx()
  {
  }

  /** Constructor with spectrum number
   * @param specNo :: spectrum # of the spectrum
   */
  ISpectrum::ISpectrum(const specid_t specNo)
  : m_specNo(specNo),
    detectorIDs(), refX(), refDx()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Copy constructor
   */
  ISpectrum::ISpectrum(const ISpectrum& other)
  : m_specNo(other.m_specNo), detectorIDs(other.detectorIDs),
    refX(other.refX), refDx(other.refDx)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Copy spectrum number and detector IDs, but not X vector, from another ISpectrum
   *
   * @param other :: take the values from other, put them in this
   */
  void ISpectrum::copyInfoFrom(const ISpectrum& other)
  {
    m_specNo = other.m_specNo;
    detectorIDs = other.detectorIDs;
  }


  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ISpectrum::~ISpectrum()
  {
  }

  // =============================================================================================
  /// Sets the x data.
  /// @param X :: vector of X data
  void ISpectrum::setX(const MantidVec& X)
  {  refX.access()=X; }

  /// Sets the x error data.
  /// @param Dx :: vector of X error data
  void ISpectrum::setDx(const MantidVec& Dx)
  {  refDx.access()=Dx; }


  /// Sets the x data.
  /// @param X :: vector of X data
  void ISpectrum::setX(const MantidVecPtr& X)
  { refX=X; }

  /// Sets the x error data.
  /// @param Dx :: vector of X error data
  void ISpectrum::setDx(const MantidVecPtr& Dx)
  { refDx=Dx; }


  /// Sets the x data
  void ISpectrum::setX(const MantidVecPtr::ptr_type& X)
  { refX=X; }

  /// Sets the x data error
  /// @param X :: vector of X data
  /// @param Dx :: vector of X error data
  void ISpectrum::setDx(const MantidVecPtr::ptr_type& Dx)
  {refDx=Dx;}


  // =============================================================================================
  ///Returns the x data
  MantidVec& ISpectrum::dataX()
  { return refX.access(); }

  ///Returns the x error data
  MantidVec& ISpectrum::dataDx()
  { return refDx.access(); }

  /// Returns the x data const
  const MantidVec& ISpectrum::dataX() const
  { return *refX; }

  /// Returns the x error data const
  const MantidVec& ISpectrum::dataDx() const
  { return *refDx; }

  /// Returns a pointer to the x data
  MantidVecPtr ISpectrum::ptrX() const
  { return refX; }



  // =============================================================================================
  // --------------------------------------------------------------------------
  /** Add a detector ID to the set of detector IDs
   *
   * @param detID :: detector ID to insert in set.
   */
  void ISpectrum::addDetectorID(const detid_t detID)
  {
    this->detectorIDs.insert( detID );
  }

  /** Add a set of detector IDs to the set of detector IDs
   *
   * @param detID :: set of detector IDs to insert in set.
   */
  void ISpectrum::addDetectorIDs(const std::set<detid_t> detIDs)
  {
    if (detIDs.size() == 0) return;
    this->detectorIDs.insert( detIDs.begin(), detIDs.end() );
  }

  /** Add a vector of detector IDs to the set of detector IDs
   *
   * @param detID :: vector of detector IDs to insert in set.
   */
  void ISpectrum::addDetectorIDs(const std::vector<detid_t> detIDs)
  {
    if (detIDs.size() == 0) return;
    this->detectorIDs.insert( detIDs.begin(), detIDs.end() );
  }

  // --------------------------------------------------------------------------
  /** Clear the list of detector IDs, then add one.
   *
   * @param detID :: detector ID to insert in set.
   */
  void ISpectrum::setDetectorID(const detid_t detID)
  {
    this->detectorIDs.clear();
    this->detectorIDs.insert( detID );
  }

  // --------------------------------------------------------------------------
  /** Return true if the given detector ID is in the list for this ISpectrum */
  bool ISpectrum::hasDetectorID(const detid_t detID) const
  {
    return ( detectorIDs.find(detID) != detectorIDs.end() );
  }

  // --------------------------------------------------------------------------
  /** Get a const reference to the detector IDs set.
   */
  const std::set<detid_t>& ISpectrum::getDetectorIDs() const
  {
    return this->detectorIDs;
  }

  // --------------------------------------------------------------------------
  /** Clear the detector IDs set.
   */
  void ISpectrum::clearDetectorIDs()
  {
    this->detectorIDs.clear();
    return;
  }

  // --------------------------------------------------------------------------
  /** Get a mutable reference to the detector IDs set.
   */
  std::set<detid_t>& ISpectrum::getDetectorIDs()
  {
    return this->detectorIDs;
  }



  // ---------------------------------------------------------
  /// @return the spectrum number of this spectrum
  specid_t ISpectrum::getSpectrumNo() const
  {
    return m_specNo;
  }

  /** Sets the the spectrum number of this spectrum
   * @param num :: the spectrum number of this spectrum */
  void ISpectrum::setSpectrumNo(specid_t num)
  {
    m_specNo = num;
  }


} // namespace Mantid
} // namespace API

