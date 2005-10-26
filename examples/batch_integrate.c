/*
  Last changed Time-stamp: <2005-10-26 17:50:18 raim>
  $Id: batch_integrate.c,v 1.14 2005/10/26 15:53:16 raimc Exp $
*/
/* 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY, WITHOUT EVEN THE IMPLIED WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. The software and
 * documentation provided hereunder is on an "as is" basis, and the
 * authors have no obligations to provide maintenance, support,
 * updates, enhancements or modifications.  In no event shall the
 * authors be liable to any party for direct, indirect, special,
 * incidental or consequential damages, including lost profits, arising
 * out of the use of this software and its documentation, even if the
 * authors have been advised of the possibility of such damage.  See
 * the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * The original code contained here was initially developed by:
 *
 *     Rainer Machne
 *
 * Contributor(s):
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/sbmlsolver/odeSolver.h"


int
main (int argc, char *argv[]){
  int i, j;
  char model[256];
  char parameter[256];
  char *reaction;
  double start, end, steps, value;
  double time = 0.0;
  double printstep = 1.0;
  SBMLDocument_t *d;
  SBMLReader_t *sr;

  /* required SOSlib structures, that will be created
     during and must be freed at the end of this program  */
  cvodeSettings_t *set;
  varySettings_t *vs;
  SBMLResultsMatrix_t *resM;
  SBMLResults_t *results;
   
  sscanf(argv[1], "%s", model);
  sscanf(argv[2], "%lf", &time);
  sscanf(argv[3], "%lf", &printstep);
  sscanf(argv[4], "%lf", &start);
  sscanf(argv[5], "%lf", &end);
  sscanf(argv[6], "%lf", &steps);
  strcpy(parameter, argv[7]);
  
  if ( argc > 8 ) {
    ASSIGN_NEW_MEMORY_BLOCK(reaction, strlen(argv[8])+1, char, 0);
    sprintf(reaction, argv[8]);
  }
  else{
    reaction = NULL;
  }
  
  printf("### Varying parameter %s (reaction %s) from %f to %f in %f steps\n",
	 parameter, reaction, start, end, steps);
  
  /* parsing the SBML model with libSBML */
  sr = SBMLReader_create();
  d = SBMLReader_readSBML(sr, model);
  SBMLReader_free(sr);  


  /* Setting SBML ODE Solver integration parameters */
  /* Setting SBML ODE Solver integration parameters with default values */
  set = CvodeSettings_create();
  /* resetting the values we need */
  CvodeSettings_setTime(set, time, printstep);
  CvodeSettings_setErrors(set, 1e-18, 1e-10, 10000);
  CvodeSettings_setSwitches(set, 1, 0, 1, 1, 1, 0); 
  
  /* Setting SBML Ode Solver batch integration parameters */
  vs = VarySettings_allocate(1, steps+1);
  VarySettings_addParameter(vs, parameter, reaction, start, end);
  VarySettings_dump(vs);

  if ( reaction != NULL )
    free(reaction);

  /* calling the SBML ODE Solver Batch function,
     and retrieving SBMLResults */
  resM = SBML_odeSolverBatch(d, set, vs);

  if ( resM == NULL ) {
    printf("### Parameter variation not succesful!\n");
    return(0);
  }
    /* we don't need these anymore */
  CvodeSettings_free(set);  
  /* printf("%s", writeSBMLToString(d)); */
  SBMLDocument_free(d);
  VarySettings_free(vs);

  results = resM->results[0][0];

  for ( i=0; i<resM->i; i++ ) {
    for ( j=0; j<resM->j; j++ ) {
      results = SBMLResultsMatrix_getResults(resM, i, j);
      printf("### RESULTS Parameter %d, Step %d \n", i+1, j+1);
      /* printing results only for species*/
      SBMLResults_dumpSpecies(results);
    }
  }

  SBMLResultsMatrix_free(resM);
  return (EXIT_SUCCESS);  
}


/* End of file */
