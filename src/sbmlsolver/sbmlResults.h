/*
  Last changed Time-stamp: <2005-10-17 15:47:07 raim>
  $Id: sbmlResults.h,v 1.4 2005/10/17 16:07:50 raimc Exp $
*/
#ifndef _SBMLRESULTS_H_
#define _SBMLRESULTS_H_

#include "sbmlsolver/exportdefs.h"

/* A simple structure containing variable names,
   and their time courses generated by simulation routines.
*/
typedef struct timeCourseT timeCourseT_t ;

struct timeCourseT {
  int timepoints;  /* number of timepoints, including initial
		      conditions (formerly known as `nout +1') */
  char *name;    /* variable names */
  double *values; /* variable values  x:timepoints, y:num_val */
} ;

/* A simple structure containing variable names,
   and their time courses generated by simulation routines.
*/
typedef struct timeCourse timeCourse_t ;

struct timeCourse {
  int timepoints;  /* number of timepoints, including initial
		      conditions (formerly known as `nout +1') */
  int num_val;     /* number of variables */
  char **names;    /* variable names */
  double **values; /* variable values  x:timepoints, y:num_val */  
} ;


/* A simple structure that contains time courses - represented
   by the TimeCourse structure - for
   SBML structures, such as species, non-constant compartments
   and parameters and reaction fluxes.
*/

typedef struct _SBMLResults SBMLResults_t;
struct _SBMLResults {
  int timepoints; /* number of timepoints for which results exist,
		     including initial conditions */
  double *time;

  /* concentration and variable parameter and compartment time series */
  timeCourse_t *species;       /* time courses for all species */  
  timeCourse_t *compartments;  /* time courses for all non-constant
			       compartments */
  timeCourse_t *parameters;    /* time courses for all non-constant
			     global parameters */
  /* flux time series */
  timeCourse_t *fluxes;   
} ;



typedef struct _SBMLResultsMatrix SBMLResultsMatrix_t;
struct _SBMLResultsMatrix {
  SBMLResults_t ***results;
  int i;
  int j;
} ;


SBML_ODESOLVER_API SBMLResults_t *SBMLResults_create(Model_t *, int timepoints);
SBML_ODESOLVER_API void SBMLResults_free(SBMLResults_t *);

SBMLResultsMatrix_t *SBMLResultsMatrix_allocate(int values, int timepoints);

SBML_ODESOLVER_API void SBMLResultsMatrix_free(SBMLResultsMatrix_t *);

SBML_ODESOLVER_API SBMLResults_t *SBMLResultsMatrix_getResults(SBMLResultsMatrix_t *, int i, int j);

#endif

/* End of file */
