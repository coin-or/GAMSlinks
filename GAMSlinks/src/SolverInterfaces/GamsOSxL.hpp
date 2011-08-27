// Copyright (C) GAMS Development and others 2008-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSOSXL_HPP_
#define GAMSOSXL_HPP_

#include <cstdlib>
#include <string>

class OSInstance;
class OSnLNode;
class OSResult;

struct gmoRec;
struct gevRec;

/** converting between GAMS Modeling Object (GMO) and OS entities (OSiL, OSrL) */
class GamsOSxL {
private:
   struct gmoRec*        gmo;                /**< GAMS modeling object */
   struct gevRec*        gev;                /**< GAMS environment */

   bool                  gmo_is_our;         /**< whether the object owns the GMO object */

   OSnLNode* parseGamsInstructions(
      int                codelen,            /**< length of GAMS instructions */
      int*               opcodes,            /**< opcodes of GAMS instructions */
      int*               fields,             /**< fields of GAMS instructions */
      int                constantlen,        /**< length of GAMS constants pool */
      double*            constants           /**< GAMS constants pool */
   );

   OSInstance*           osinstance;         /**< Optimization Services Instance object */

public:
   GamsOSxL(
      struct gmoRec*     gmo_ = NULL         /**< GAMS modeling object, or NULL if set later */
   )
   : gmo(NULL),
     gev(NULL),
     gmo_is_our(false),
     osinstance(NULL)
   {
      if( gmo_ != NULL )
         setGMO(gmo_);
   }

   /** constructor for creating an GMO object from a compiled GAMS model */
   GamsOSxL(
      const char*        datfile             /**< name of file with compiled GAMS model */
   )
   : gmo(NULL),
     gev(NULL),
     gmo_is_our(false),
     osinstance(NULL)
   {
      initGMO(datfile);
   }

   ~GamsOSxL();

   /** sets Gams Modeling Object
    * Can only be used if not GMO has been set already.
    */
   void setGMO(
      struct gmoRec*     gmo_                /**< GAMS modeling object */
   );

   /** initializes GMO from a file containing a compiled GAMS model */
   bool initGMO(
      const char*        datfile             /**< name of file with compiled GAMS model */
   );

   /** creates an OSInstance from the GAMS modeling object
    * returns whether the instance has been created successfully
    */
   bool createOSInstance();

   /** returns OSInstance and moves ownership to calling function
    * this object forgets about the instance
    */
   OSInstance* takeOverOSInstance()
   {
      OSInstance* osinstance_ = osinstance;
      osinstance = NULL;
      return osinstance_;
   }

   /** returns OSInstances but keeps ownership
    * destructor will free OSInstance
    */
   OSInstance* getOSInstance()
   {
      return osinstance;
   }

   /** stores a solution in GMO with the result given as OSResult object */
   void writeSolution(
      OSResult&          osresult            /**< optimization result as OSResult object */
   );

   /** stores a solution in GMO with the result given as osrl string */
   void writeSolution(
      std::string&       osrl                /**< optimization result as string in OSrL format */
   );
};

#endif /*GAMSOSXL_HPP_*/
