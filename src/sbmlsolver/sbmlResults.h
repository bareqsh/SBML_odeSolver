/*
  Last changed Time-stamp: <2005-10-19 18:21:46 raim>
  $Id: sbmlResults.h,v 1.5 2005/10/19 16:39:43 raimc Exp $
*/
#ifndef _SBMLRESULTS_H_
#define _SBMLRESULTS_H_

#include "sbmlsolver/exportdefs.h"

/* A simple structure containing a variable name,
   and its time courses generated by simulation routines.
*/
typedef struct timeCourse timeCourse_t ;
struct timeCourse {
  int timepoints;  /* number of timepoints, including initial
		      conditions */
  char *name;      /* variable name */
  double *values;  /* variable time course */
} ;

/* A simple structure containing num_val timeCourses */
typedef struct timeCourseArray timeCourseArray_t ;
struct timeCourseArray {
  int num_val;        /* number of time courses  */
  timeCourse_t **tc;  /* time courses */
} ;


/* A simple structure that contains time courses - represented
   by the TimeCourseArray structure - for
   SBML structures, such as species, non-constant compartments
   and parameters and reaction fluxes.
*/

typedef struct _SBMLResults SBMLResults_t;
struct _SBMLResults {

  timeCourse_t *time;                /* the time points */

  /* concentration and variable parameter and compartment time series */
  timeCourseArray_t *species;       /* time courses for all species */  
  timeCourseArray_t *compartments;  /* time courses for all non-constant
				       compartments */
  timeCourseArray_t *parameters;    /* time courses for all non-constant
				       global parameters */
  /* flux time series */
  timeCourseArray_t *fluxes;   
} ;



typedef struct _SBMLResultsMatrix SBMLResultsMatrix_t;
struct _SBMLResultsMatrix {
  SBMLResults_t ***results;
  int i;
  int j;
} ;

SBML_ODESOLVER_API timeCourse_t *SBMLResults_getTime(SBMLResults_t *);
SBML_ODESOLVER_API timeCourse_t *SBMLResults_getTimeCourse(const char *, SBMLResults_t *);
SBML_ODESOLVER_API timeCourse_t *Compartment_getTimeCourse(Compartment_t *, SBMLResults_t *);
SBML_ODESOLVER_API timeCourse_t *Species_getTimeCourse(Species_t *, SBMLResults_t *);
SBML_ODESOLVER_API timeCourse_t *Parameter_getTimeCourse(Parameter_t *, SBMLResults_t *);
SBML_ODESOLVER_API const char*TimeCourse_getName(timeCourse_t *);
SBML_ODESOLVER_API int TimeCourse_getNumValues(timeCourse_t *);
SBML_ODESOLVER_API double TimeCourse_getValue(timeCourse_t *, int);
SBML_ODESOLVER_API void SBMLResults_dump(SBMLResults_t *);
SBML_ODESOLVER_API void SBMLResults_free(SBMLResults_t *);
SBML_ODESOLVER_API void SBMLResultsMatrix_free(SBMLResultsMatrix_t *);
SBML_ODESOLVER_API SBMLResults_t *SBMLResultsMatrix_getResults(SBMLResultsMatrix_t *, int i, int j);
SBML_ODESOLVER_API void TimeCourseArray_dump(timeCourseArray_t *, timeCourse_t *time);


/* not part of the API */
SBMLResults_t *SBMLResults_create(Model_t *, int timepoints);
SBMLResultsMatrix_t *SBMLResultsMatrix_allocate(int values, int timepoints);

#endif

/* End of file */
