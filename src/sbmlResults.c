/*
  Last changed Time-stamp: <2005-05-26 13:29:30 raim>
  $Id: sbmlResults.c,v 1.1 2005/05/30 19:49:12 raimc Exp $
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sbml/SBMLTypes.h>
#include <sbml/common/common.h>

#include "sbmlResults.h"

static TimeCourse
TimeCourse_create(int names, int timepoints);
static void
TimeCourse_free(TimeCourse tc);

/* The function
   SBMLResults SBMLResults_create(Model_t *m, int timepoints)
   allocates memory for simulation results (time courses) mapped back
   on SBML structures (i.e. species, and non-constant compartments
   and parameters. It takes CvodeData as an argument, that has to
   contain CVODE integraton results.
*/

SBMLResults
SBMLResults_create(Model_t *m, int timepoints){

  int i, num_reactions, num_species, num_compartments, num_parameters;
  Species_t *s;
  Compartment_t *c;
  Parameter_t *p;
  Reaction_t *r;
  SBMLResults results;

  if(!(results = (SBMLResults) safe_calloc(1, sizeof(*results)))){
    fprintf(stderr, "failed!\n");
  }
  
  results->timepoints = timepoints;
  
  /* Allocating the time array:
     number of output times, plus 1 for the initial values. */
  if(!(results->time =
       (double *)safe_calloc(timepoints, sizeof(double)))) {
    fprintf(stderr, "failed!\n");
  }
  
  /* Allocating arrays for all SBML species */
  num_species = Model_getNumSpecies(m);
  results->species = TimeCourse_create(num_species, timepoints);
  /* Writing species names */
  for ( i=0; i<Model_getNumSpecies(m); i++) {
    s = Model_getSpecies(m, i);
    results->species->names[i] =
        (char *)calloc(strlen(Species_getId(s))+1, sizeof(char));
    strcpy(results->species->names[i], Species_getId(s));
  }

  /* Allocating arrays for all variable SBML compartments */
  num_compartments = 0;
  for ( i=0; i<Model_getNumCompartments(m); i++ ) {
    if ( ! Compartment_getConstant(Model_getCompartment(m, i)) ) {
      num_compartments++;
    }
  }
  results->compartments = TimeCourse_create(num_compartments, timepoints);
  /* Writing variable compartment names */
  for ( i=0; i<Model_getNumCompartments(m); i++) {
    c = Model_getCompartment(m, i);
    if ( ! Compartment_getConstant(c) ) {
      results->compartments->names[i] =
        (char *)calloc(strlen(Compartment_getId(c))+1, sizeof(char));
      strcpy(results->compartments->names[i], Compartment_getId(c));
    }
  }

  /* Allocating arrays for all variable SBML parameters */
  num_parameters = 0;
  for ( i=0; i<Model_getNumParameters(m); i++ ) {
    if ( ! Parameter_getConstant(Model_getParameter(m, i)) ) {
      num_parameters++;
    }
  }    
  results->parameters = TimeCourse_create(num_parameters, timepoints);
  /* Writing variable parameter names */
  for ( i=0; i<Model_getNumParameters(m); i++) {
    p = Model_getParameter(m, i);
    if ( ! Parameter_getConstant(p) ) {
      results->parameters->names[i] =
        (char *)calloc(strlen(Parameter_getId(p))+1, sizeof(char));
      strcpy(results->parameters->names[i], Parameter_getId(p));
    }
  }  

  /* Allocating arrays for all variable SBML reactions */
  num_reactions = Model_getNumReactions(m);
  results->fluxes = TimeCourse_create(num_reactions, timepoints);
  /* Writing reaction names */
  for ( i=0; i<Model_getNumReactions(m); i++ ) {
    r = Model_getReaction(m, i);
    results->fluxes->names[i] =
        (char *)calloc(strlen(Reaction_getId(r))+1, sizeof(char));
    strcpy(results->fluxes->names[i], Reaction_getId(r));
  }

  return results;
}

void
SBMLResults_free(SBMLResults results) {

  if ( results != NULL ) {
    if ( results->time != NULL ) {
      free(results->time);
    }
    /* see SBMLResults_create and header file for comments */
    TimeCourse_free(results->species);
    TimeCourse_free(results->compartments);
    TimeCourse_free(results->parameters);
    TimeCourse_free(results->fluxes);

    free(results);
  }
}

static TimeCourse
TimeCourse_create(int num_val, int timepoints){

  int i;
  TimeCourse tc;

  /* timecourse variables */  
  if(!(tc = (TimeCourse)safe_calloc(1, sizeof(struct _TimeCourse)))){
    fprintf(stderr, "failed!\n");
  }
  tc->num_val = num_val;
  tc->timepoints = timepoints;
  if(!(tc->names =  (char **)safe_calloc(num_val, sizeof(char*)))){
    fprintf(stderr, "failed!\n");
  }  
  if(!(tc->values =  (double **)safe_calloc(timepoints, sizeof(double*)))){
    fprintf(stderr, "failed!\n");
  }
  for ( i=0; i<tc->timepoints; i++ ) {
    if(!(tc->values[i] =
	 (double *)safe_calloc(num_val, sizeof(double)))){
      fprintf(stderr, "failed!\n");
    }
  }
  return tc;
}

static void
TimeCourse_free(TimeCourse tc) {

  int i;

  if ( tc != NULL ) {
    if ( tc->names != NULL ) {
      for ( i=0; i<tc->num_val; i++ ){
	if ( tc->names[i] != NULL ) {
	  free(tc->names[i]);
	}
      }  
      free(tc->names);
    }
    if ( tc->values != NULL ) {
      for ( i=0; i<tc->timepoints; i++ ){
	if ( tc->values[i] != NULL ) {
	  free(tc->values[i]);
	}
      }  
      free(tc->values);
    }
    free(tc);
  }
}



/* End of file */
