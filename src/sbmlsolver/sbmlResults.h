/*
  Last changed Time-stamp: <2005-10-11 17:47:22 raim>
  $Id: sbmlResults.h,v 1.3 2005/10/12 12:52:09 raimc Exp $
*/
#ifndef _SBMLRESULTS_H_
#define _SBMLRESULTS_H_

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

SBMLResults_t *
SBMLResults_create(Model_t *m, int timepoints);
void
SBMLResults_free(SBMLResults_t *results);

#endif

/* End of file */
