/*
  Last changed Time-stamp: <2007-09-28 14:15:50 raim>
  $Id: integratorinstance.c,v 1.1 2007/09/28 13:14:16 raimc Exp $
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

#include <sbmlsolver/odeSolver.h>

int main (void)
{
  cvodeSettings_t *options = CvodeSettings_createWithTime(10000, 1000);
  odeModel_t *odemodel = ODEModel_createFromFile("MAPK.xml");
  variableIndex_t *vi = ODEModel_getVariableIndex(odemodel, "MAPK_PP");
  
  integratorInstance_t *ii = IntegratorInstance_create(odemodel, options);
  printf("# Time Course for variable %s \n",
	 ODEModel_getVariableName(odemodel, vi));
  
  while( ! IntegratorInstance_timeCourseCompleted(ii) )
    if ( IntegratorInstance_integrateOneStep(ii ) )
      printf("%g %g\n", IntegratorInstance_getTime(ii),
	     IntegratorInstance_getVariableValue(ii, vi));
    else
      break;
  SolverError_dump();
  ODEModel_free(odemodel);
  VariableIndex_free(vi);
  IntegratorInstance_free(ii);
  
  return (EXIT_SUCCESS);  
}


/* End of file */
