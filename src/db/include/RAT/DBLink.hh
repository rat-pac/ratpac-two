/** @class DBLink
 *  Link to RATDB table
 *
 *  @author Stan Seibert <volsung@physics.utexas.edu>
 *
 *  The DBLink class provides indirect access to a table in RATDB.
 *  If you are familiar with SQL databases, the link is very much like
 *  a cursor.  Normally, you will not construct a DBLink object
 *  yourself, but rather obtain a "smart pointer" to one via the
 *  DB::GetLink() method.  Once you have a DBLinkPtr, you can
 *  use it just like a normal pointer to a DBLink object.
 *
 *  In order to access a field in the table, you will use one of the
 *  Get methods.  You must know the data type of the field you are
 *  trying to access.  If you try to retrieve a float field using
 *  GetI(), then a DBNotFoundError exception will be thrown.  (In
 *  principle that means you could have two fields of different types
 *  with the same name, but you should NEVER EVER DO THAT.)
 *
 *  Implementation notes:
 *
 *  Note that none of the Get methods in this class are declared @c
 *  const to allow for the possibility of caching the results inside
 *  this object.
 *
 *  The reason for the indirection of a link to a table is to allow
 *  for more flexibility implementing the backends, which will someday
 *  be a mixture of text files and a SQL database.  The link will then
 *  be able to provide local caching (for speed) and the ability to
 *  move around in time ("show me the data I would have gotten on
 *  Feb. 3").  None of that fanciness exists yet, so for now you will
 *  have to be content with the awesome power of being able to load an
 *  array.
 *
 */

#ifndef __RAT_DBLink__
#define __RAT_DBLink__

#include <RAT/DB.hh>
#include <RAT/DBTable.hh>
#include <string>
#include <vector>

namespace RAT {

class DB;

class DBLink {
 public:
  /** Create a new link to a table.
   *
   *  @param  db  Pointer to database which created this link.  When
   *              the link is destroyed, the database will be
   *              notified.
   *  @param  tblname  Name of table
   *  @param  index    Index of table
   *  @param  currentRun Run number to use for validity range checking
   */
  DBLink(DB *db, std::string tblname, std::string index, int currentRun = 1);

  /** Destroy link.
   *
   *  When this object is destroyed, it will call DB::RemoveLink()
   *  on the database which created it.
   */
  ~DBLink();

  /** Get name of table this link points to. */
  inline std::string GetName() const { return tblname; };
  /** Get index of table this link points to. */
  inline std::string GetIndex() const { return index; };

  /** Set the current run number for fetching run-specific tables from the
   * server */
  void SetCurrentRun(int runNumber) { currentRun = runNumber; };

  /** Retrieve integer field.
   *
   *  @throws DBNotFoundException if integer field @p name does not
   *  exist.
   */
  int GetI(const std::string &name);

  /** Retrieve double field.
   *
   *  @throws DBNotFoundException if double field @p name does not
   *  exist.
   */
  double GetD(const std::string &name);

  /** Retrieve string field.
   *
   *  @throws DBNotFoundException if string field @p name does not
   *  exist.
   */
  std::string GetS(const std::string &name);

  /** Retrieve bool field.
   *
   *  @throws DBNotFoundException if string field @p name does not
   *  exist.
   */
  bool GetZ(const std::string &name);

  /** Retrieve integer array field.
   *
   *  @throws DBNotFoundException if integer array field @p name
   *  does not exist.
   */
  std::vector<int> GetIArray(const std::string &name);

  /** Retrieve float array field.
   *
   *  @throws DBNotFoundException if float array field @p name
   *  does not exist.
   */
  std::vector<float> GetFArrayFromD(const std::string &name);
  std::vector<float> DArrayToFArray(const std::vector<double> &input);

  /** Retrieve double array field.
   *
   *  @throws DBNotFoundException if double array field @p name
   *  does not exist.
   */
  std::vector<double> GetDArray(const std::string &name);

  /** Retrieve string array field.
   *
   *  @throws DBNotFoundException if string array field @p name
   *  does not exist.
   */
  std::vector<std::string> GetSArray(const std::string &name);

  /** Retrieve bool array field.
   *
   *  @throws DBNotFoundException if string array field @p name
   *  does not exist.
   */
  std::vector<bool> GetZArray(const std::string &name);

  /** Retrieve raw JSON value.
   *
   *  @throws DBNotFoundException if field @p name
   *  does not exist.
   */
  json::Value GetJSON(const std::string &name);

  /** Get the value of a field from the table, including plane
   *  precedence rules.
   *
   *  @param  T          C++ data type for field
   *  @param  fieldname  Name of field
   */
  template <class T>
  T Get(const std::string &fieldname);

  // Used by DB class, do not use this yourself
  void Unlink() { db = 0; };

 protected:
  /** Pointer to DB which created this link. */
  DB *db;

  /** Name of table this link refers to. */
  std::string tblname;

  /** Index of table this link refers to */
  std::string index;

  /** Current run number */
  int currentRun;
};

template <class T>
T DBLink::Get(const std::string &fieldname) {
  DBTable *tbl;
  // First try user plane
  tbl = db->GetUserTable(tblname, index);
  if (!tbl || tbl->GetFieldType(fieldname) == DBTable::NOTFOUND) {
    // Then try the run plane
    tbl = db->GetRunTable(tblname, index, currentRun);
    if (tbl) {
      if (tbl->GetFieldType(fieldname) == DBTable::NOTFOUND) throw DBNotFoundError(tblname, index, fieldname);
    } else {
      // Finally try default plane
      tbl = db->GetDefaultTable(tblname, index);
      if (!tbl || tbl->GetFieldType(fieldname) == DBTable::NOTFOUND) {
        throw DBNotFoundError(tblname, index, fieldname);
      }
    }
  }

  // Make class explicit to satisfy Sun CC 5.3
  T value = tbl->DBTable::Get<T>(fieldname);

  // Trace DB accesses
  Log::TraceDBAccess(tblname, index, fieldname, value);

  return value;
}

}  // namespace RAT

#endif
