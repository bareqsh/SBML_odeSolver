#include <stdio.h>
#include <malloc.h>

#include "sbmlsolver/odeModel.h"
#include "sbmlsolver/integratorInstance.h"
#include "sbmlsolver/solverError.h"

void DumpState(
    integratorInstance_t *iia,
    integratorInstance_t *iib,
    variableIndex_t *v1, variableIndex_t *v2
    )
{
    printf(
        " %g %g %g %g %g\n", 
        IntegratorInstance_getTime(iia),
        IntegratorInstance_getVariableValue(iia, v1),
        IntegratorInstance_getVariableValue(iia, v2),
        IntegratorInstance_getVariableValue(iib, v1),
        IntegratorInstance_getVariableValue(iib, v2));
}

int doit(void)
{
    int i ;
    double value;
    cvodeSettings_t *settings ;
    variableIndex_t *s1, *s2;
    integratorInstance_t *integratorInstanceA;
    integratorInstance_t *integratorInstanceB;

    odeModel_t *model = ODEModel_createFromFile("basic-model1-forward-l2.xml", 1);
    RETURN_ON_ERRORS_WITH(1);

    s1 = ODEModel_getVariableIndex(model, "S1");
    s2 = ODEModel_getVariableIndex(model, "S2");
    RETURN_ON_ERRORS_WITH(1);

    /* Creating settings with default values */
    settings = CvodeSettings_create();

    
    /* Setting end time to .1, number of time steps to 1 and NULL
       instead of an optional predefined time series (double *); due
       to Indefinitely == 1, Printstep 1 will be ignored and Time =
       0.1 will be used as timestep for infinite integration */
    CvodeSettings_setTime(settings, .1, 1);

    /* Setting Cvode Parameters: absolute and relative tolerances and
       maximal internal step */
    CvodeSettings_setErrors(settings, 1e-18, 1e-14, 500);

    
    /* Setting Integration Switches:       
    settings->UseJacobian = 1;  toggle use of Jacobian ASTs (1) or
                                internal approximation  (0)
    settings->Indefinitely = 1; run without a defined end time, Time
			        field contains step duration, ignore
			        PrintStep field
    settings->HaltOnEvent = 0;  doesn't stops integration upon an event
    settings->SteadyState = 0;  don't stop integration upon a steady state 
    settings->StoreResults = 0; don't Store time course history
    */
    CvodeSettings_setSwitches(settings, 1, 1, 0, 0, 0);


    /* Generate two independent integrator instances from the same
       odeModel and cvodeSettings */
    integratorInstanceA = IntegratorInstance_create(model, settings);
    RETURN_ON_ERRORS_WITH(1);

    integratorInstanceB = IntegratorInstance_create(model, settings);
    RETURN_ON_ERRORS_WITH(1);

    DumpState(integratorInstanceA, integratorInstanceB, s1, s2);

    for (i=0; i != 30; i++)
    {

        /* run integrations A and B */
        IntegratorInstance_integrateOneStep(integratorInstanceA);
        RETURN_ON_ERRORS_WITH(1);

        IntegratorInstance_integrateOneStep(integratorInstanceB);
        RETURN_ON_ERRORS_WITH(1);

	/* While variable s1 from integration A is between 1e-15 and
	   1e-16, set variables s1 and s2 in integration B to some
	   value.  This function also takes care of creating and
	   freeing CVODE solver structures when ODE variables are
	   changed!
	*/
	value = IntegratorInstance_getVariableValue(integratorInstanceA, s1);
        if ( value < 1.e-15 && value >  1.e-16 )
        {
            IntegratorInstance_setVariableValue(integratorInstanceB,
						s1,1.5e-15);
            IntegratorInstance_setVariableValue(integratorInstanceB,
						s2,1.5e-15);
        }

        DumpState(integratorInstanceA, integratorInstanceB, s1, s2);

    }

    IntegratorInstance_free(integratorInstanceA);
    IntegratorInstance_free(integratorInstanceB);
    VariableIndex_free(s1);
    VariableIndex_free(s2);
    ODEModel_free(model);
    CvodeSettings_free(settings);

    return 0;
}

int main (int argc, char *argv[])
{
    int result = doit();
    char *errors = SolverError_dumpToString();

    fprintf(stderr, errors);
    SolverError_freeDumpString(errors);

    return result;
}
