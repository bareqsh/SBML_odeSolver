/*
  Last changed Time-stamp: <2005-12-12 16:19:53 raim>
  $Id: processAST.c,v 1.24 2005/12/12 15:21:25 raimc Exp $
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
 *     Stefan M�ller
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sbml/SBMLTypes.h>

/* System specific definitions,
   created by configure script */
#ifndef WIN32
#include "config.h"
#endif

#include "sbmlsolver/cvodedata.h"
#include "sbmlsolver/processAST.h"
#include "sbmlsolver/ASTIndexNameNode.h"
#include "sbmlsolver/solverError.h"

#ifdef WIN32
double acosh(double x)
{
	return log(x + (sqrt(x - 1) * sqrt(x + 1)));
}

double asinh(double x)
{
	return log(x + sqrt((x * x) + 1));
}

double atanh(double x)
{
	return (log(1 + x) - log(1-x))/2 ;
}
#endif /* WIN32 */

/**
 * function pointer to user defined function
 */
static double
(*UsrDefFunc)(char*, int, double*) = NULL;

/**
 * sets function pointer for  user defined function to udf
 */
SBML_ODESOLVER_API void
setUserDefinedFunction(double(*udf)(char*, int, double*))
{
  UsrDefFunc = udf;
}

#if OLD_LIBSBML
#define AST_FUNCTION_DELAY AST_NAME_DELAY
#endif

/* ------------------------------------------------------------------------ */

/** Copies the passed constant AST, including potential
    SOSlib ASTNodeIndex, and returns the copy.
*/

ASTNode_t *copyAST(const ASTNode_t *f)
{
  int i;
  ASTNode_t *copy;

  copy = ASTNode_create();

  /* DISTINCTION OF CASES */

  /* integers, reals */
  if ( ASTNode_isInteger(f) ) {
    ASTNode_setInteger(copy, ASTNode_getInteger(f));
  }
  else if ( ASTNode_isReal(f) ) {
    ASTNode_setReal(copy, ASTNode_getReal(f));
  }  
  /* variables */
  else if ( ASTNode_isName(f) ) {
    if ( ASTNode_isSetIndex((ASTNode_t *)f) ) {
        ASTNode_free(copy);
	copy = ASTNode_createIndexName();
	ASTNode_setIndex(copy, ASTNode_getIndex((ASTNode_t *)f));
    }
    ASTNode_setName(copy, ASTNode_getName(f));
  }
  /* constants */
  /* functions, operators */
  else {
    ASTNode_setType(copy, ASTNode_getType(f));
    /* user-defined functions: name must be set */
    if ( ASTNode_getType(f) == AST_FUNCTION ) {
      ASTNode_setName(copy, ASTNode_getName(f));
    }
    for ( i=0; i<ASTNode_getNumChildren(f); i++ ) {
      ASTNode_addChild(copy, copyAST(ASTNode_getChild(f,i)));
    }
  }

  return copy;
}

/* ------------------------------------------------------------------------ */

/* takes an AST and a string array and converts AST_NAME to
   AST_IndexName, which holds the name and the index in the array */
ASTNode_t *indexAST(const ASTNode_t *f, int nvalues, char **names)
{
  int i, found;
  ASTNode_t *index;

  index = ASTNode_create();

  /* DISTINCTION OF CASES */

  /* integers, reals */
  if ( ASTNode_isInteger(f) ) {
    ASTNode_setInteger(index, ASTNode_getInteger(f));
  }
  else if ( ASTNode_isReal(f) ) {
    ASTNode_setReal(index, ASTNode_getReal(f));
  }
  /* copy existing indexes */
/*   else if ( ASTNode_isSetIndex(f) ) { */
/*     ASTNode_free(index); */
/*     index = ASTNode_createIndexName(); */
/*     ASTNode_setName(index, ASTNode_getName(f)); */
/*     ASTNode_setIndex(index, ASTNode_getIndex(f));     */
/*   } */
  /* writing indexed name nodes */
  else if ( ASTNode_isName(f) ) {
    found = 0;
    for ( i=0; i<nvalues; i++ ) {
      if ( strcmp(ASTNode_getName(f), names[i]) == 0 ) {
        ASTNode_free(index);
	index = ASTNode_createIndexName(); /*!memory leak in sensitivity.c!*/
	ASTNode_setIndex(index, i);
	ASTNode_setName(index, ASTNode_getName(f));
	found++;
      }
    }
    if ( !found )
      ASTNode_setName(index, ASTNode_getName(f));
  }
  /* constants */
  /* functions, operators */
  else {
    ASTNode_setType(index, ASTNode_getType(f));
    /* user-defined functions: name must be set */
    if ( ASTNode_getType(f) == AST_FUNCTION ) {
      ASTNode_setName(index, ASTNode_getName(f));
    }
    for ( i=0; i<ASTNode_getNumChildren(f); i++ ) {
      ASTNode_addChild(index, indexAST(ASTNode_getChild(f,i), nvalues, names));
    }
  }

  return index;
}

/* ------------------------------------------------------------------------ */

/*
  The function "static double evaluateAST(ASTNode_t *n, N_Vector y,
  char **species, int neq)" evaluates the formula of an Abstract
  Syntax Tree by simple recursion and returns the result as a double
  value.
  Variable names are searched in passed 'data', which is
  an SBML_odeSolver specific data structure, containing variable
  and parameter names and their current values. See files cvodedata.c/h
  and odeConstruct.c to learn how this structure is built.
  
  Not implemented:
  * PIECEWISE, LAMBDA, and the SBML model specific function DELAY 
  * Complex numbers and/or checking for domains of trigonometric and root
    functions.
  * Checking for precision and rounding errors.
  * User-defined functions (SBML Function Definitions)
  
  The Nodetypes AST_TIME, AST_DELAY and AST_PIECEWISE default to 0.
  The SBML DELAY function and unknown functions (SBML user-defined
  functions) use the value of the left child (first argument to
  function) or 0 if the node has no children.
*/

SBML_ODESOLVER_API double evaluateAST(ASTNode_t *n, cvodeData_t *data)
{
  int i, j, childnum;
  int found;
  int true;
 
  ASTNodeType_t type;
  /* ASTNode_t **child; */

  char *unknown;
  double result;

  if ( n == NULL ) {
    SolverError_error(FATAL_ERROR_TYPE,
		      SOLVER_ERROR_AST_UNKNOWN_NODE_TYPE,
		      "evaluateAST: empty Abstract Syntax Tree (AST).");
    return (0);
  }
  if ( ASTNode_isUnknown(n) ) {
    SolverError_error(FATAL_ERROR_TYPE,
		      SOLVER_ERROR_AST_UNKNOWN_NODE_TYPE,
		      "evaluateAST: unknown ASTNode type");
  }
  result = 0;

  childnum = ASTNode_getNumChildren(n);
  type = ASTNode_getType(n);

 
  switch(type)
    {
    case AST_INTEGER:
      result = (double) ASTNode_getInteger(n);      
      break;
    case AST_REAL:
      result = ASTNode_getReal(n);
      break;
    case AST_REAL_E:
      result = ASTNode_getReal(n);
      break;      
    case AST_RATIONAL:
      result = ASTNode_getReal(n) ;
      break;
	
    case AST_NAME:
      /*
	Finds the value of the variable in
	the data->value array. SOSlib's extension to libSBML's
	AST allows to add the index of the variable in this array
	to AST_NAME (ASTIndexName). If the ASTNode is node is not
	indexed, its array index is searched via the model->names
	array, that corresponds to the value array. For nodes
        with name `Time' or `time' the data->currenttime is returned.
	If no value is found a fatal error is produced. */
      found = 0;
      if ( ASTNode_isSetIndex(n) ) {
	result = data->value[ASTNode_getIndex(n)];
	found++;
      }
      if ( found == 0 ) {
	if ( strcmp(ASTNode_getName(n),"time") == 0 ||
	     strcmp(ASTNode_getName(n),"Time") == 0 ||
	     strcmp(ASTNode_getName(n),"TIME") == 0 ) {
	  result = (double) data->currenttime;
	  found++;
	}
      }
      if ( found == 0 ) {
	for ( j=0; j<data->nvalues; j++ ) {
	  if ( (strcmp(ASTNode_getName(n),data->model->names[j]) == 0) ) {
	    result = data->value[j];
	    found++;
	  }
	}
      }
      if ( found == 0 ) {
        SolverError_error(FATAL_ERROR_TYPE,
			  SOLVER_ERROR_AST_EVALUATION_FAILED_MISSING_VALUE,
			  "No value found for AST_NAME %s . Defaults to Zero "
			  "to avoid program crash", ASTNode_getName(n));
	result = 0;
      }
      break;
    case AST_FUNCTION_DELAY:
      SolverError_error(FATAL_ERROR_TYPE,
			SOLVER_ERROR_AST_EVALUATION_FAILED_DELAY,
			"Solving ODEs with Delay is not implemented. "
			"Defaults to 0 to avoid program crash");
      result = 0.0;
      break;
    case AST_NAME_TIME:
      result = (double) data->currenttime;
      break;
	
    case AST_CONSTANT_E:
      /* exp(1) is used to adjust exponentiale to machine precision */
      result = exp(1);
      break;
    case AST_CONSTANT_FALSE:
      result = 0.0;
      break;
    case AST_CONSTANT_PI:
      /* pi = 4 * atan 1  is used to adjust Pi to machine precision */
      result = 4.*atan(1.);
      break;
    case AST_CONSTANT_TRUE:
      result = 1.0;
      break;

    case AST_PLUS:
      result = 0.0;
      for( i=0; i<childnum; i++) 
	result += evaluateAST(child(n,i),data);   
      break;      
    case AST_MINUS:
      if(childnum<2)
	result = - (evaluateAST(child(n,0),data));
      else
	result = evaluateAST(child(n,0),data) - evaluateAST(child(n,1),data);
      break;
    case AST_TIMES:
      result = 1.0;
      for( i=0; i<childnum; i++) 
	result *= evaluateAST(child(n,i),data);
      break;
    case AST_DIVIDE:
      result = evaluateAST(child(n,0),data) / evaluateAST(child(n,1),data);
      break;
    case AST_POWER:
      result = pow(evaluateAST(child(n,0),data),evaluateAST(child(n,1),data));
      break;
    case AST_LAMBDA:
      SolverError_error(FATAL_ERROR_TYPE,
			SOLVER_ERROR_AST_EVALUATION_FAILED_LAMBDA,
			"Lambda can only be used in SBML function definitions."
			" Defaults to 0 to avoid program crash");
      result = 0.0;
      break;
      
    case AST_FUNCTION:
      if (UsrDefFunc == NULL) {
	SolverError_error(FATAL_ERROR_TYPE,
			  SOLVER_ERROR_AST_EVALUATION_FAILED_FUNCTION,
			  "The function %s() has not been defined "
			  "in the SBML input model or as an externally "
			  "supplied function. Defaults to 0 to avoid "
			  "program crash",
			  ASTNode_getName(n));
	result = 0.0;
      }
      else {
	/* 	   "Evaluating external function\n"); */
	double *func_vals = NULL;
	ASSIGN_NEW_MEMORY_BLOCK(func_vals, childnum+1, double, 0);
	for ( i=0; i<childnum; i++ ) {
	  func_vals[i] = evaluateAST(child(n,i), data);
	}
	result = UsrDefFunc((char *)ASTNode_getName(n), childnum, func_vals);
	free(func_vals);
      }
      break;
    case AST_FUNCTION_ABS:
      result = (double) fabs(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_ARCCOS:
      result = acos(evaluateAST(child(n,0),data)) ;
      break;
    case AST_FUNCTION_ARCCOSH:
      result = acosh(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_ARCCOT:
      /* arccot x =  arctan (1 / x) */
      result = atan(1./ evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_ARCCOTH:
      /* arccoth x = 1/2 * ln((x+1)/(x-1)) */
      result = ((1./2.)*log((evaluateAST(child(n,0),data)+1.)/
			    (evaluateAST(child(n,0),data)-1.)) );
      break;
    case AST_FUNCTION_ARCCSC:
      /* arccsc(x) = Arctan(1 / sqrt((x - 1)(x + 1))) */
      result = atan( 1. / MySQRT( (evaluateAST(child(n,0),data)-1.)*
				(evaluateAST(child(n,0),data)+1.) ) );
      break;
    case AST_FUNCTION_ARCCSCH:
      /* arccsch(x) = ln((1 + sqrt(1 + x^2)) / x) */
      result = log((1.+MySQRT((1+MySQR(evaluateAST(child(n,0),data))))) /
		   evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_ARCSEC:
      /* arcsec(x) = arctan(sqrt((x - 1)(x + 1))) */   
      result = atan( MySQRT( (evaluateAST(child(n,0),data)-1.)*
			    (evaluateAST(child(n,0),data)+1.) ) );
      break;
    case AST_FUNCTION_ARCSECH:
      /* arcsech(x) = ln((1 + sqrt(1 - x^2)) / x) */
      result = log((1.+pow((1-MySQR(evaluateAST(child(n,0),data))),0.5))/
		   evaluateAST(child(n,0),data));      
      break;
    case AST_FUNCTION_ARCSIN:
      result = asin(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_ARCSINH:
      result = asinh(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_ARCTAN:
      result = atan(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_ARCTANH:
      result = atanh(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_CEILING:
      result = ceil(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_COS:
      result = cos(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_COSH:
      result = cosh(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_COT:
      /* cot x = 1 / tan x */
      result = (1./tan(evaluateAST(child(n,0),data)));
      break;
    case AST_FUNCTION_COTH:
      /* coth x = cosh x / sinh x */
      result = cosh(evaluateAST(child(n,0),data)) /
	sinh(evaluateAST(child(n,0),data));
      break;  
    case AST_FUNCTION_CSC:
      /* csc x = 1 / sin x */
      result = (1./sin(evaluateAST(child(n,0),data)));
      break;
    case AST_FUNCTION_CSCH:
      /* csch x = 1 / sinh x  */
      result = (1./sinh(evaluateAST(child(n,0),data)));
      break;
    case AST_FUNCTION_EXP:
      result = exp(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_FACTORIAL:
      {
	int j;
	j = floor(evaluateAST(child(n,0),data));
	if ( evaluateAST(child(n,0),data) != j ) {
	  SolverError_error(ERROR_ERROR_TYPE,
			    SOLVER_ERROR_AST_EVALUATION_FAILED_FLOAT_FACTORIAL,
			    "The factorial is only implemented."
			    "for integer values. The floor value of the "
			    "passed float is used for calculation!");
	}	
	for(result=1;j>1;--j)
	  result *= j;
      }
      break;
    case AST_FUNCTION_FLOOR:
      result = floor(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_LN:
      result = log(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_LOG:
      /* log(x,y) = log10(y)/log10(x) (where x is the base)  */
      result = log10(evaluateAST(child(n,1),data)) /
	log10(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_PIECEWISE:
      if ( evaluateAST(child(n,0),data) ) {
	result = evaluateAST(child(n,1),data);
      }
      else {
	result = evaluateAST(child(n,2),data);
      }
      break;
    case AST_FUNCTION_POWER:
      result = pow(evaluateAST(child(n,0),data),evaluateAST(child(n,1),data));
      break;
    case AST_FUNCTION_ROOT:
      result = pow(evaluateAST(child(n,1),data),
		   (1./evaluateAST(child(n,0),data)));
      break;
    case AST_FUNCTION_SEC:
      /* sec x = 1 / cos x */
      result = 1./cos(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_SECH:
      /* sech x = 1 / cosh x */
      result = 1./cosh(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_SIN:
      result = sin(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_SINH:
      result = sinh(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_TAN:
      result = tan(evaluateAST(child(n,0),data));
      break;
    case AST_FUNCTION_TANH:
      result = tanh(evaluateAST(child(n,0),data));
      break;

    case AST_LOGICAL_AND:
      true = 0;
      for ( i=0; i<childnum; i++ ) {
	true += evaluateAST(child(n,i),data);
      }
      if ( true == childnum ) {
	result = 1.0;
      }
      else {
	result = 0.0;
      }
      break;
    case AST_LOGICAL_NOT:
      result = (double) (!(evaluateAST(child(n,0),data)));
      break;
    case AST_LOGICAL_OR:
      true = 0;
      for ( i=0; i<childnum; i++ ) {
	true += evaluateAST(child(n,i),data);
      }
      if ( true > 0 ) {
	result = 1.0;
      }
      else {
	result = 0.0;
      }
      break;
    case AST_LOGICAL_XOR:
      /*!!! check if defined for more than three childs !!!*/
      true = 0;
      for ( i=0; i<childnum; i++ ) {
	true += evaluateAST(child(n,i),data);
      }
      if ( true == 1 || true == 3 ) {
	result = 1.0;
      }
      else {
	result = 0.0;
      }
      break;
      
    case AST_RELATIONAL_EQ :
      result = (double) ((evaluateAST(child(n,0),data)) ==
			 (evaluateAST(child(n,1),data)));
      break;
    case AST_RELATIONAL_GEQ:
      result = (double) ((evaluateAST(child(n,0),data)) >=
			 (evaluateAST(child(n,1),data)));
      break;
    case AST_RELATIONAL_GT:
      result = (double) ((evaluateAST(child(n,0),data)) >
			 (evaluateAST(child(n,1),data)));
      break;
    case AST_RELATIONAL_LEQ:
      result = (double) ((evaluateAST(child(n,0),data)) <=
			 (evaluateAST(child(n,1),data)));
      break;
    case AST_RELATIONAL_LT :
      result = (double) ((evaluateAST(child(n,0),data)) <
			 (evaluateAST(child(n,1),data)));
      break;

    default:
      result = 0;
      break;
    }
  
  return result;
}

/* ------------------------------------------------------------------------ */

/* logical predicate that checks if ASTNode is a user-defined variable name,
   (or time), or a user defined function */
int user_defined(ASTNode_t *node) {
  if ( ASTNode_isName(node) || ASTNode_getType(node)==AST_FUNCTION )
    return 1;
  return 0;
}

/* ------------------------------------------------------------------------ */

/* The function differentiateAST(f,x)
   returns the derivative of the passed AST f
   with respect to the passed variable x,
   using basic differentiation rules. */

SBML_ODESOLVER_API ASTNode_t *differentiateAST(ASTNode_t *f, char *x) {

  int i, j, childnum;
  int found;
  ASTNodeType_t type;
  ASTNode_t *fprime, *helper, *simple;
  ASTNode_t *help_1, *help_2, *help_3;
  ASTNode_t *prod, *sum, *tmp;
  List_t *list;
  char *fname, *dfname;
  
  fprime = ASTNode_create();

  /* check if variable x is part of formula */  
  found = 0;
  list = ASTNode_getListOfNodes(f, (ASTNodePredicate) user_defined);
  for ( i=0; i<List_size(list); i++ ) {
    if ( strcmp(ASTNode_getName(List_get(list, i)), x) == 0 ) {
      found = 1;
    }
  }
  List_free(list);

  if ( found == 0 ) {
    ASTNode_setReal(fprime, 0.0);
    return fprime;
  }

  /* helpful information */
  type = ASTNode_getType(f);
  childnum = ASTNode_getNumChildren(f);

  /* DISTINCTION OF CASES */

  /* integers, reals */
  if ( ASTNode_isNumber(f) ) { /* redundant */
    ASTNode_setReal(fprime, 0.0);
  }
  /* variables (or time) */ 
  else if ( ASTNode_isName(f) ) {
    if ( strcmp(ASTNode_getName(f), x) == 0 ) { /* redundant */
      ASTNode_setReal(fprime, 1.0);    
    }
    else {
      ASTNode_setReal(fprime, 0.0);
    }   
  }
  /* constants */
  else if ( ASTNode_isConstant(f) ) { /* redundant */
    switch(type) {
    case AST_CONSTANT_E:
    case AST_CONSTANT_PI:
      ASTNode_setReal(fprime, 0.0);
      break;
    case AST_CONSTANT_TRUE:
    case AST_CONSTANT_FALSE:
      ASTNode_setType(fprime, type);
      break;
    default:
      SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_AST_DIFFERENTIATION_FAILED_CONSTANT,
                        "differentiateAST: constant: impossible case");
      ASTNode_setName(fprime, "differentiation_failed");
    }
  }
  /* operators */
  else if ( ASTNode_isOperator(f) || type==AST_FUNCTION_POWER ) {
    switch(type) {
    case AST_PLUS:
      ASTNode_setType(fprime, AST_PLUS);
      for ( i=0; i<childnum; i++ ) {
	ASTNode_addChild(fprime,
			 differentiateAST(ASTNode_getChild(f, i),x));
      }
      break;
    case AST_MINUS:
      ASTNode_setType(fprime, AST_MINUS);
      for ( i=0; i<childnum; i++ ) {
	ASTNode_addChild(fprime,
			 differentiateAST(ASTNode_getChild(f, i),x));
      }
      break;
    case AST_TIMES:
      /* catch cases with operand number != 2 */
      if ( ASTNode_getNumChildren(f) != 2 ) {
	helper = simplifyAST(f); /* decomposes the n-ary operator */
	ASTNode_free(fprime);
	fprime = differentiateAST(helper, x);
	ASTNode_free(helper);
      }
      else {
	/* f(x)=a(x)*b(x) => f'(x) = a'*b + a*b' */
	ASTNode_setType(fprime, AST_PLUS);    

	ASTNode_addChild(fprime, ASTNode_create());
	help_1 = ASTNode_getChild(fprime, 0);
	ASTNode_setType (help_1, AST_TIMES);
	ASTNode_addChild(help_1, differentiateAST(ASTNode_getChild(f, 0), x));
	ASTNode_addChild(help_1, copyAST(ASTNode_getChild(f, 1)));

	ASTNode_addChild(fprime, ASTNode_create());
	help_1 = ASTNode_getChild(fprime, 1);
	ASTNode_setType (help_1, AST_TIMES);
	ASTNode_addChild(help_1, copyAST(ASTNode_getChild(f, 0)));
	ASTNode_addChild(help_1, differentiateAST(ASTNode_getChild(f, 1), x));
      }
      break;
    case AST_DIVIDE:
      /* f(x)=a(x)/b(x) => f'(x) = a'/b - a/b^2*b' */    
      ASTNode_setType(fprime, AST_MINUS);

      ASTNode_addChild(fprime, ASTNode_create());
      help_1 = ASTNode_getChild(fprime, 0);
      ASTNode_setType (help_1, AST_DIVIDE);
      ASTNode_addChild(help_1, differentiateAST(ASTNode_getChild(f, 0), x));
      ASTNode_addChild(help_1, copyAST(ASTNode_getChild(f, 1)));

      ASTNode_addChild(fprime, ASTNode_create());
      help_1 = ASTNode_getChild(fprime, 1);
      ASTNode_setType (help_1, AST_TIMES);
      ASTNode_addChild(help_1, ASTNode_create());
      ASTNode_addChild(help_1, differentiateAST(ASTNode_getChild(f, 1), x));

      help_2 = ASTNode_getChild(help_1, 0);
      ASTNode_setType (help_2, AST_DIVIDE);
      ASTNode_addChild(help_2, copyAST(ASTNode_getChild(f, 0)));
      ASTNode_addChild(help_2, ASTNode_create());

      help_3 = ASTNode_getChild(help_2, 1);
      ASTNode_setType (help_3, AST_POWER);
      ASTNode_addChild(help_3, copyAST(ASTNode_getChild(f, 1)));
      ASTNode_addChild(help_3, ASTNode_create());
      ASTNode_setInteger(ASTNode_getChild(help_3, 1), 2);
      break;
    case AST_POWER:
    case AST_FUNCTION_POWER:
      /* f(x)=a(x)^b => f'(x) = b * a^(b-1)*a' */
      /* check if variable x is part of b(x) */
      found = 0;  
      list = ASTNode_getListOfNodes(ASTNode_getChild(f, 1),
				    (ASTNodePredicate) user_defined);
      for ( i=0; i<List_size(list); i++ ) {
	if ( strcmp(ASTNode_getName(List_get(list, i)), x) == 0 ) {
	  found = 1;
	}    
      }
      List_free(list);
      if ( found == 0 ) {
	ASTNode_setType (fprime, AST_TIMES);

	ASTNode_addChild(fprime, copyAST(ASTNode_getChild(f, 1)));
	ASTNode_addChild(fprime, ASTNode_create());

	help_1 = ASTNode_getChild(fprime, 1);
	ASTNode_setType (help_1, AST_TIMES);
	ASTNode_addChild(help_1, ASTNode_create());
	ASTNode_addChild(help_1, differentiateAST(ASTNode_getChild(f, 0), x));

	help_2 = ASTNode_getChild(help_1, 0);
	ASTNode_setType (help_2, AST_POWER);
	ASTNode_addChild(help_2, copyAST(ASTNode_getChild(f, 0)));
	ASTNode_addChild(help_2, ASTNode_create());

	help_3 = ASTNode_getChild(help_2, 1);
	ASTNode_setType (help_3, AST_MINUS);
	ASTNode_addChild(help_3, copyAST(ASTNode_getChild(f, 1)));
	ASTNode_addChild(help_3, ASTNode_create());
	ASTNode_setReal(ASTNode_getChild(help_3, 1), 1.0);	
	break;
      }
      /* f(x)=a^b(x) => f'(x) = f * ln(a)*b' */
      /* check if variable x is part of a(x) */
      found = 0;  
      list = ASTNode_getListOfNodes(ASTNode_getChild(f, 0),
				    (ASTNodePredicate) user_defined);
      for ( i=0; i<List_size(list); i++ ) {
	if ( strcmp(ASTNode_getName(List_get(list, i)), x) == 0 ) {
	  found = 1;
	}    
      }
      List_free(list);
      if ( found == 0 ) {
	ASTNode_setType (fprime, AST_TIMES);
	ASTNode_addChild(fprime, copyAST(f));
	ASTNode_addChild(fprime, ASTNode_create());
	help_1 = ASTNode_getChild(fprime, 1);
	ASTNode_setType (help_1, AST_TIMES);
	ASTNode_addChild(help_1, ASTNode_create());
	ASTNode_addChild(help_1, differentiateAST(ASTNode_getChild(f, 1), x));
	help_2 = ASTNode_getChild(help_1, 0);
	ASTNode_setType (help_2, AST_FUNCTION_LN);
	ASTNode_addChild(help_2, copyAST(ASTNode_getChild(f, 0)));
	break;
      }
      /* f(x)=a(x)^b(x) => f'(x)= f * ( b/a*a' + ln(a)*b' ) */
      ASTNode_setType (fprime, AST_TIMES);
      ASTNode_addChild(fprime, copyAST(f));
      ASTNode_addChild(fprime, ASTNode_create());
      help_1 = ASTNode_getChild(fprime, 1);
      ASTNode_setType (help_1, AST_PLUS);
      ASTNode_addChild(help_1, ASTNode_create());
      ASTNode_addChild(help_1, ASTNode_create());
      help_2 = ASTNode_getChild(help_1, 0);
      ASTNode_setType (help_2, AST_TIMES);
      ASTNode_addChild(help_2, ASTNode_create());
      ASTNode_addChild(help_2, differentiateAST(ASTNode_getChild(f, 0), x));
      help_3 = ASTNode_getChild(help_2, 0);
      ASTNode_setType (help_3, AST_DIVIDE);
      ASTNode_addChild(help_3, copyAST(ASTNode_getChild(f, 1)));
      ASTNode_addChild(help_3, copyAST(ASTNode_getChild(f, 0)));
      help_2 = ASTNode_getChild(help_1, 1);
      ASTNode_setType (help_2, AST_TIMES);
      ASTNode_addChild(help_2, ASTNode_create());
      ASTNode_addChild(help_2, differentiateAST(ASTNode_getChild(f, 1), x));
      help_3 = ASTNode_getChild(help_2, 0);
      ASTNode_setType (help_3, AST_FUNCTION_LN);
      ASTNode_addChild(help_3, copyAST(ASTNode_getChild(f, 0)));
      break;
    default:
      SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_AST_DIFFERENTIATION_FAILED_OPERATOR,
                        "differentiateAST: operator: impossible case");
      ASTNode_setName(fprime, "differentiation_failed");
    }
  }
  /* functions */
  else if ( ASTNode_isFunction(f) || type==AST_LAMBDA ) {
    switch(type) {
    case AST_LAMBDA:
      SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_AST_DIFFERENTIATION_FAILED_LAMBDA,
                        "differentiateAST: lambda: not implemented");
      ASTNode_setName(fprime, "differentiation_failed");
      break;
    case AST_FUNCTION:
      fname  = (char *) ASTNode_getName(f);
      /* feature for inverse problems:
	 differentiation for (sic!) a user-defined function */
      if ( strcmp(fname, x) == 0 ) {
	ASTNode_setType (fprime, AST_FUNCTION);	
	ASSIGN_NEW_MEMORY_BLOCK(dfname, strlen(fname)+3, char, NULL);
	sprintf(dfname, "d_%s", fname);
	ASTNode_setName(fprime, dfname);
	free(dfname);
	for ( j=0; j<childnum; j++ ) {
	  ASTNode_addChild(fprime, copyAST(ASTNode_getChild(f, j)));
	}
	break;
      }
      /* differentiation of a user-defined function for a variable */
      ASTNode_free(fprime); /* not needed yet */ 
      sum = tmp = NULL;
      /* loop over arguments */
      for ( i=0; i<childnum; i++ ) {
	/* does argument depend on x? */
	found = 0;  
	list = ASTNode_getListOfNodes(ASTNode_getChild(f, i),
				      (ASTNodePredicate) user_defined);
	for ( j=0; j<List_size(list); j++ ) {
	  if ( strcmp(ASTNode_getName(List_get(list, j)), x) == 0 ) {
	    found = 1;
	  }    
	}
	List_free(list);
	/* calculate summands */
	prod = ASTNode_create();
	if ( found == 0 ) { /* argument does not depend on x */
	  ASTNode_setReal(prod, 0.0);
	}
	else { /* argument depends on x */
	  ASTNode_setType (prod, AST_TIMES);
	  ASTNode_addChild(prod, ASTNode_create());
	  ASTNode_addChild(prod, differentiateAST(ASTNode_getChild(f, i) , x));
	  helper = ASTNode_getChild(prod, 0);
	  ASTNode_setType (helper, AST_FUNCTION);
	  ASSIGN_NEW_MEMORY_BLOCK(dfname, strlen(fname)+5, char, NULL);
	  sprintf(dfname, "d%i_%s", i, fname);
	  ASTNode_setName(helper, dfname);
	  free(dfname);
	  for ( j=0; j<childnum; j++ ) {
	    ASTNode_addChild(helper, copyAST(ASTNode_getChild(f, j)));
	  }
	}
	if ( i==0 ) { /* first summand */
	  sum = prod;
	}
	else { /* other summands */
	  sum = ASTNode_create();
	  ASTNode_setType (sum, AST_PLUS);
	  ASTNode_addChild(sum, tmp);
	  ASTNode_addChild(sum, prod);
	}
	tmp = sum;
      }
      /* result */
      fprime = sum;
      break;
    case AST_FUNCTION_ABS:                                         /* WRONG */
      /* f(x)=abs(a(x)) => f' = sig(a)*a'
	 CAN RESULT IN A DISCONTINUOUS FUNCTION! */ 
      ASTNode_setType(fprime, ASTNode_getType(f)); /* WRONG !!! */
      ASTNode_addChild(fprime, differentiateAST(ASTNode_getChild(f,0),x));
      break;

    /* TRIGONOMETRIC FUNCTION DERIVATIVES
       mostly taken from
       http://www.rism.com/Trig/circular.htm and
       http://www.rism.com/Trig/hyperbol.htm */  
    case AST_FUNCTION_ARCCOS:
      /* f(x)=arccos(a(x)) => f' = - a' / sqrt(1 - a^2)  */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  - a'  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /*  sqrt(...)  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_FUNCTION_ROOT);
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger(child2(fprime,1,0), 2);
      /* 1 - a^2) */
      /*  1  - */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType( child2(fprime,1,1), AST_MINUS );
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setInteger(child3(fprime,1,1,0), 1);
      /*  a^2  */
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setType(  child3(fprime,1,1,1), AST_POWER);
      ASTNode_addChild( child3(fprime,1,1,1),
			copyAST(ASTNode_getChild(f,0)));
      ASTNode_addChild( child3(fprime,1,1,1),
			ASTNode_create());
      ASTNode_setInteger(ASTNode_getChild(child3(fprime,1,1,1),1), 2);
      break;
    case AST_FUNCTION_ARCCOSH:
      /* f(x)=arccosh(a(x)) => f' =   a' / sqrt(a^2 - 1)  */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  a'  */
      ASTNode_addChild(fprime, differentiateAST(ASTNode_getChild(f,0),x));
      /*  sqrt(...)  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_FUNCTION_ROOT);
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger(child2(fprime,1,0), 2);
      /* a^2 - 1 */
      /*  a^2  - */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType( child2(fprime,1,1), AST_MINUS );
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setType(child3(fprime,1,1,0), AST_POWER);
      ASTNode_addChild(child3(fprime,1,1,0),
		       copyAST(ASTNode_getChild(f,0)));
    
      ASTNode_addChild(child3(fprime,1,1,0),
		       ASTNode_create());
      ASTNode_setInteger(ASTNode_getChild(child3(fprime,1,1,0),1), 2);
      /*  1  */
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setInteger(child3(fprime,1,1,1), 1);
      break;
    case AST_FUNCTION_ARCCOT:
      /* f(x)=arccot(a(x)) => f' = - a' / (1 + a^2) */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  - a'  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /*  1 + a^2  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_PLUS);
      /*  1  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger( child2(fprime,1,0), 1);
      /*  a^2  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(  child2(fprime,1,1), AST_POWER);
      ASTNode_addChild( child2(fprime,1,1),
			copyAST(ASTNode_getChild(f,0)));
      ASTNode_addChild( child2(fprime,1,1),
			ASTNode_create());
      ASTNode_setInteger( child3(fprime,1,1,1), 2);     
      break;
    case AST_FUNCTION_ARCCOTH:
      /* f(x)=arccoth(a(x)) => f' = - a' / (-1 + a^2) */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  - a'  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /*  -1 + a^2  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_PLUS);
      /*  -1  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,0), AST_MINUS);
      ASTNode_addChild(child2(fprime,1,0), ASTNode_create());
      ASTNode_setInteger(child3(fprime,1,0,0), 1);
      /*  a^2  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(  child2(fprime,1,1), AST_POWER);
      ASTNode_addChild( child2(fprime,1,1),
			copyAST(ASTNode_getChild(f,0)));
      ASTNode_addChild( child2(fprime,1,1),
			ASTNode_create());
      ASTNode_setInteger( child3(fprime,1,1,1), 2);    
      break;
    case AST_FUNCTION_ARCCSC:
      /* f(x)=arccsc(a(x)) => f' = - a' * a * sqrt(a^2 - 1)  */
      ASTNode_setType(fprime,AST_TIMES);
      /*  - a' * ... */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /* ... a * ... */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_TIMES);    
      ASTNode_addChild(ASTNode_getChild(fprime,1),
		       copyAST(ASTNode_getChild(f,0)));    
      /*  sqrt(...)  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,1), AST_FUNCTION_ROOT);    
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setInteger(child3(fprime,1,1,0), 2);
      /* a^2 - 1 */
      /*  a^2  - */
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setType(child3(fprime,1,1,1), AST_MINUS );
    
      ASTNode_addChild(child3(fprime,1,1,1), ASTNode_create());    
      ASTNode_setType(ASTNode_getChild(child3(fprime,1,1,1),0),
		      AST_POWER);
   
      ASTNode_addChild(ASTNode_getChild(child3(fprime,1,1,1),0),
		       copyAST(ASTNode_getChild(f,0)));
    
      ASTNode_addChild(ASTNode_getChild(child3(fprime,1,1,1),0),
		       ASTNode_create());
      ASTNode_setInteger(child2(child3(fprime,1,1,1),0,1),
			 2);
      /*  1  */
      ASTNode_addChild(child3(fprime,1,1,1), ASTNode_create());
      ASTNode_setInteger(ASTNode_getChild(child3(fprime,1,1,1),1), 1);
      break;
    case AST_FUNCTION_ARCCSCH:
      /* f(x)=arccos(a(x)) => f' = - a' * a * sqrt(a^2 + 1)  */
      ASTNode_setType(fprime,AST_TIMES);
      /*  - a' * ... */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /* ... a * ... */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_TIMES);    
      ASTNode_addChild(ASTNode_getChild(fprime,1),
		       copyAST(ASTNode_getChild(f,0)));    
      /*  sqrt(...)  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,1), AST_FUNCTION_ROOT);    
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setInteger(child3(fprime,1,1,0), 2);
      /* a^2 + 1 */
      /*  a^2  + */
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setType(child3(fprime,1,1,1), AST_PLUS );
    
      ASTNode_addChild(child3(fprime,1,1,1), ASTNode_create());    
      ASTNode_setType(ASTNode_getChild(child3(fprime,1,1,1),0),
		      AST_POWER);
   
      ASTNode_addChild(ASTNode_getChild(child3(fprime,1,1,1),0),
		       copyAST(ASTNode_getChild(f,0)));
    
      ASTNode_addChild(ASTNode_getChild(child3(fprime,1,1,1),0),
		       ASTNode_create());
      ASTNode_setInteger(child2(child3(fprime,1,1,1),0,1),
			 2);
      /*  1  */
      ASTNode_addChild(child3(fprime,1,1,1), ASTNode_create());
      ASTNode_setInteger(ASTNode_getChild(child3(fprime,1,1,1),1), 1);
      break;
    case AST_FUNCTION_ARCSEC:
      /* f(x)=arcsec(a(x)) => f' = a' * a * sqrt(a^2 - 1)  */
      ASTNode_setType(fprime,AST_TIMES);
      /*  a' * ... */
      ASTNode_addChild(fprime,
		       differentiateAST(ASTNode_getChild(f,0),x));
      /* ... a * ... */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_TIMES);    
      ASTNode_addChild(ASTNode_getChild(fprime,1),
		       copyAST(ASTNode_getChild(f,0)));    
      /*  sqrt(...)  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,1), AST_FUNCTION_ROOT);    
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setInteger(child3(fprime,1,1,0), 2);
      /* a^2 - 1 */
      /*  a^2  - */
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setType(child3(fprime,1,1,1), AST_MINUS );
    
      ASTNode_addChild(child3(fprime,1,1,1), ASTNode_create());    
      ASTNode_setType(ASTNode_getChild(child3(fprime,1,1,1),0),
		      AST_POWER);
   
      ASTNode_addChild(ASTNode_getChild(child3(fprime,1,1,1),0),
		       copyAST(ASTNode_getChild(f,0)));
    
      ASTNode_addChild(ASTNode_getChild(child3(fprime,1,1,1),0),
		       ASTNode_create());
      ASTNode_setInteger(child2(child3(fprime,1,1,1),0,1),
			 2);
      /*  1  */
      ASTNode_addChild(child3(fprime,1,1,1), ASTNode_create());
      ASTNode_setInteger(ASTNode_getChild(child3(fprime,1,1,1),1), 1);
      break;
    case AST_FUNCTION_ARCSECH:
      /* f(x)=arcsech(a(x)) => f' = - a' * a * sqrt(1 - a^2)  */
      ASTNode_setType(fprime,AST_TIMES);
      /*  - a' * ... */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /* ... a * ... */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_TIMES);    
      ASTNode_addChild(ASTNode_getChild(fprime,1),
		       copyAST(ASTNode_getChild(f,0)));    
      /*  sqrt(...)  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,1), AST_FUNCTION_ROOT);    
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setInteger(child3(fprime,1,1,0), 2);
      /*  1  - */
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setType(child3(fprime,1,1,1), AST_MINUS );
      ASTNode_addChild(child3(fprime,1,1,1), ASTNode_create());
      ASTNode_setInteger(ASTNode_getChild(child3(fprime,1,1,1),0), 1);
      /*  a^2  */   
      ASTNode_addChild(child3(fprime,1,1,1), ASTNode_create());    
      ASTNode_setType(ASTNode_getChild(child3(fprime,1,1,1),1),
		      AST_POWER);
   
      ASTNode_addChild(ASTNode_getChild(child3(fprime,1,1,1),1),
		       copyAST(ASTNode_getChild(f,0)));
    
      ASTNode_addChild(ASTNode_getChild(child3(fprime,1,1,1),1),
		       ASTNode_create());
      ASTNode_setInteger(child2(child3(fprime,1,1,1),1,1),
			 2);
      break;
    case AST_FUNCTION_ARCSIN:
      /* f(x)=arcsin(a(x)) => f' = a' / sqrt(1 - a^2)  */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  a'  */
      ASTNode_addChild(fprime,
		       differentiateAST(ASTNode_getChild(f,0),x));
      /*  sqrt(...)  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_FUNCTION_ROOT);
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger(child2(fprime,1,0), 2);
      /* 1 - a^2) */
      /*  1  - */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType( child2(fprime,1,1), AST_MINUS );
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setInteger(child3(fprime,1,1,0), 1);
      /*  a^2  */
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setType(  child3(fprime,1,1,1), AST_POWER);
      ASTNode_addChild( child3(fprime,1,1,1),
			copyAST(ASTNode_getChild(f,0)));
      ASTNode_addChild( child3(fprime,1,1,1),
			ASTNode_create());
      ASTNode_setInteger(ASTNode_getChild(child3(fprime,1,1,1),1), 2);
      break;
    case AST_FUNCTION_ARCSINH:
      /* f(x)=arcsinh(a(x)) => f' = a' / sqrt(1 + a^2)  */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  a'  */
      ASTNode_addChild(fprime,
		       differentiateAST(ASTNode_getChild(f,0),x));
      /*  sqrt(...)  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_FUNCTION_ROOT);
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger(child2(fprime,1,0), 2);
      /* 1 + a^2) */
      /*  1  + */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType( child2(fprime,1,1), AST_PLUS );
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setInteger(child3(fprime,1,1,0), 1);
      /*  a^2  */
      ASTNode_addChild(child2(fprime,1,1), ASTNode_create());
      ASTNode_setType(  child3(fprime,1,1,1), AST_POWER);
      ASTNode_addChild( child3(fprime,1,1,1),
			copyAST(ASTNode_getChild(f,0)));
      ASTNode_addChild( child3(fprime,1,1,1),
			ASTNode_create());
      ASTNode_setInteger(ASTNode_getChild(child3(fprime,1,1,1),1), 2);
      break;
    case AST_FUNCTION_ARCTAN:
      /* f(x)=atan(a(x)) => f' = a' / (1 + a^2) */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  a'  */
      ASTNode_addChild(fprime,
		       differentiateAST(ASTNode_getChild(f,0),x));
      /*  1 + a^2  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_PLUS);
      /*  1  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger( child2(fprime,1,0), 1);
      /*  a^2  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(  child2(fprime,1,1), AST_POWER);
      ASTNode_addChild( child2(fprime,1,1),
			copyAST(ASTNode_getChild(f,0)));
      ASTNode_addChild( child2(fprime,1,1),
			ASTNode_create());
      ASTNode_setInteger( child3(fprime,1,1,1), 2);    
      break;
    case AST_FUNCTION_ARCTANH:
      /* f(x)=atan(a(x)) => f' = a' / (1 - a^2) */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  a'  */
      ASTNode_addChild(fprime,
		       differentiateAST(ASTNode_getChild(f,0),x));
      /*  1 - a^2  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_MINUS);
      /*  1  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger( child2(fprime,1,0), 1);
      /*  a^2  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(  child2(fprime,1,1), AST_POWER);
      ASTNode_addChild( child2(fprime,1,1),
			copyAST(ASTNode_getChild(f,0)));
      ASTNode_addChild( child2(fprime,1,1),
			ASTNode_create());
      ASTNode_setInteger( child3(fprime,1,1,1), 2);     
      break;
    case AST_FUNCTION_CEILING:
      /* f(x) = ceil(a(x)) */
      ASTNode_setType(fprime, ASTNode_getType(f));
      ASTNode_addChild(fprime, differentiateAST(ASTNode_getChild(f,0),x));
      break;
    case AST_FUNCTION_COS:
      /* f(x)=cos(a(x)) => f' = a' * -sin(a) */   
      ASTNode_setType(fprime,AST_TIMES);
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0), ASTNode_create());      
      ASTNode_setType(child2(fprime,0,0),
		      AST_FUNCTION_SIN);
      ASTNode_addChild(child2(fprime,0,0),
		       copyAST(ASTNode_getChild(f,0)));      
      ASTNode_addChild(fprime,differentiateAST(ASTNode_getChild(f,0),x));
      break;
    case AST_FUNCTION_COSH:
      /* f(x)=cosh(a(x)) => f' = a' * -sinh(a) */   
      ASTNode_setType(fprime,AST_TIMES);
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0), ASTNode_create());      
      ASTNode_setType(child2(fprime,0,0),
		      AST_FUNCTION_SINH);
      ASTNode_addChild(child2(fprime,0,0),
		       copyAST(ASTNode_getChild(f,0)));      
      ASTNode_addChild(fprime,differentiateAST(ASTNode_getChild(f,0),x));
      break;
    case AST_FUNCTION_COT:
      /* f(x)=cot(a(x)) => f' = - a' / (sin(a))^2 */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  - a'  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /*  (sin(a))^2  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_POWER);
      /*  sin(a)  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,0),
		      AST_FUNCTION_SIN);
      ASTNode_addChild(child2(fprime,1,0),
		       copyAST(ASTNode_getChild(f,0)));
      /*  ^2  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger(child2(fprime,1,1), 2);  
      break;
    case AST_FUNCTION_COTH:
      /* f(x)=cot(a(x)) => f' = - a' / (sinh(a))^2 */
      ASTNode_setType(fprime,AST_DIVIDE);
      /*  - a'  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /*  (sinh(a))^2  */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_POWER);
      /*  sinh(a)  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,0),
		      AST_FUNCTION_SINH);
      ASTNode_addChild(child2(fprime,1,0),
		       copyAST(ASTNode_getChild(f,0)));
      /*  ^2  */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger(child2(fprime,1,1), 2);  
      break;  
    case AST_FUNCTION_CSC:
      /* f(x)=csc(a(x)) => f' = - a' * csc(a) * cot(a) */
      /* - a' * ... */
      ASTNode_setType(fprime,AST_TIMES);
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /* csc(a) * cot(x) */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_TIMES);
      /* csc(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,0), AST_FUNCTION_CSC);
      ASTNode_addChild(child2(fprime,1,0),
		       copyAST(ASTNode_getChild(f,0)));
      /* tan(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,1), AST_FUNCTION_COT);
      ASTNode_addChild(child2(fprime,1,1),
		       copyAST(ASTNode_getChild(f,0)));
      break;
    case AST_FUNCTION_CSCH:
      /* f(x)=csch(a(x)) => f' = - a' * csch(a) * coth(a) */
      /* - a' * ... */
      ASTNode_setType(fprime,AST_TIMES);
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /* csch(a) * coth(x) */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_TIMES);
      /* csch(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,0), AST_FUNCTION_CSCH);
      ASTNode_addChild(child2(fprime,1,0),
		       copyAST(ASTNode_getChild(f,0)));
      /* tanh(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,1), AST_FUNCTION_COTH);
      ASTNode_addChild(child2(fprime,1,1),
		       copyAST(ASTNode_getChild(f,0)));	
      break;
    case AST_FUNCTION_DELAY:
      SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_AST_DIFFERENTIATION_FAILED_DELAY,
                        "differentiateAST: delay: not implemented");
       ASTNode_setName(fprime, "differentiation_failed");
      break;
    case AST_FUNCTION_EXP:
      /* f(x)=e^a(x) =>  f' = e^a * a'  */
      ASTNode_setType(fprime, AST_TIMES);
      ASTNode_addChild(fprime, copyAST(f));
      ASTNode_addChild(fprime, differentiateAST(ASTNode_getChild(f,0),x));
      break;
    case AST_FUNCTION_FACTORIAL:
      SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_AST_DIFFERENTIATION_FAILED_FACTORIAL,
                        "differentiateAST: factorial: impossible case");
      ASTNode_setName(fprime, "differentiation_failed");
      break;
    case AST_FUNCTION_FLOOR:                                       /* WRONG */
      /* f(x) = floor(a(x)) */
      ASTNode_setType(fprime, ASTNode_getType(f));
      ASTNode_addChild(fprime, differentiateAST(ASTNode_getChild(f,0),x));
      break;
    case AST_FUNCTION_LN:
      /* f(x)=ln(a(x)) => f' = 1 / a * a' */
      ASTNode_setType (fprime, AST_TIMES);
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_addChild(fprime, differentiateAST(ASTNode_getChild(f, 0), x));
      help_1 = ASTNode_getChild(fprime, 0);
      ASTNode_setType (help_1, AST_DIVIDE);
      ASTNode_addChild(help_1, ASTNode_create());
      ASTNode_addChild(help_1, copyAST(ASTNode_getChild(f, 0)));
      help_2 = ASTNode_getChild(help_1, 0);
      ASTNode_setReal (help_2, 1.0);    
      break;
    case AST_FUNCTION_LOG:
    /* f(x)=log(b,x) = 1/ln(b)*ln(x)
       differentiate ln(x)! */
      ASTNode_setType (fprime, AST_TIMES);
      ASTNode_addChild(fprime, ASTNode_create());
      help_1 = ASTNode_getChild(fprime, 0);
      ASTNode_setType (help_1, AST_DIVIDE);
      ASTNode_addChild(help_1, ASTNode_create());
      ASTNode_addChild(help_1, ASTNode_create());
      help_2 = ASTNode_getChild(help_1, 0);
      ASTNode_setReal (help_2, 1.0);
      help_2 = ASTNode_getChild(help_1, 1);
      ASTNode_setType (help_2, AST_FUNCTION_LN);
      ASTNode_addChild(help_2, copyAST(ASTNode_getChild(f, 0)));

      helper = ASTNode_create();
      ASTNode_setType (helper, AST_FUNCTION_LN);
      ASTNode_addChild(helper, copyAST(ASTNode_getChild(f, 1)));
      
      ASTNode_addChild(fprime, differentiateAST(helper, x));
      ASTNode_free(helper);
      break;
    case AST_FUNCTION_PIECEWISE:
      SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_AST_DIFFERENTIATION_FAILED_FACTORIAL,
                        "differentiateAST: piecewise: not implemented");
      ASTNode_setName(fprime, "differentiation_failed");
      break;
    case AST_FUNCTION_ROOT:
      /* f(x)=root(a(x),b(x)) = a(x)^(1/b(x))
	 differentiate this! */
      helper = ASTNode_create();
      ASTNode_setType (helper, AST_FUNCTION_POWER);
      ASTNode_addChild(helper, copyAST(ASTNode_getChild(f,0)));
      ASTNode_addChild(helper, ASTNode_create());
      help_1 = ASTNode_getChild(helper, 1);
      ASTNode_setType (help_1, AST_DIVIDE);
      ASTNode_addChild(help_1, ASTNode_create());
      ASTNode_addChild(help_1, copyAST(ASTNode_getChild(f,1)));
      help_2 = ASTNode_getChild(help_1, 0);
      ASTNode_setReal (help_2, 1.0);

      fprime = differentiateAST(helper, x);
      ASTNode_free(helper);
      break;
    case AST_FUNCTION_SEC:
      /* f(x)=sec(a(x)) => f' = a' * sec(a) * tan(x) */
      /* a' * ... */
      ASTNode_setType(fprime,AST_TIMES);
      ASTNode_addChild(fprime, differentiateAST(ASTNode_getChild(f,0),x));
      /* sec(a) * tan(x) */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_TIMES);
      /* sec(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,0), AST_FUNCTION_SEC);
      ASTNode_addChild(child2(fprime,1,0),
		       copyAST(ASTNode_getChild(f,0)));
      /* tan(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,1), AST_FUNCTION_TAN);
      ASTNode_addChild(child2(fprime,1,1),
		       copyAST(ASTNode_getChild(f,0)));		         
      break;
    case AST_FUNCTION_SECH:
      /* f(x)=sech(a(x)) f' = - a' * sech(a) * tanh(a)  */
      ASTNode_setType(fprime,AST_TIMES);
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_MINUS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       differentiateAST(ASTNode_getChild(f,0),x));
      /* sec(a) * tan(x) */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_TIMES);
      /* sec(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,0), AST_FUNCTION_SECH);
      ASTNode_addChild(child2(fprime,1,0),
		       copyAST(ASTNode_getChild(f,0)));
      /* tan(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,1), AST_FUNCTION_TANH);
      ASTNode_addChild(child2(fprime,1,1),
		       copyAST(ASTNode_getChild(f,0)));    
      break;
    case AST_FUNCTION_SIN:
      /* f(x)=sin(a(x)) => f' = a' * cos(a) */
      ASTNode_setType(fprime,AST_TIMES);
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_FUNCTION_COS);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       copyAST(ASTNode_getChild(f,0)));      
      ASTNode_addChild(fprime,differentiateAST(ASTNode_getChild(f,0),x));
      break;
    case AST_FUNCTION_SINH:
      /* f(x)=sinh(a(x)) => f' = a' * cosh(a) */    
      ASTNode_setType(fprime,AST_TIMES);
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,0), AST_FUNCTION_COSH);
      ASTNode_addChild(ASTNode_getChild(fprime,0),
		       copyAST(ASTNode_getChild(f,0)));      
      ASTNode_addChild(fprime,differentiateAST(ASTNode_getChild(f,0),x));
      break;
    case AST_FUNCTION_TAN:
      /* f(x)= tan(a(x)) => f' = a' / (cos(a))^2  */
      ASTNode_setType(fprime,AST_DIVIDE);
      /* a' */
      ASTNode_addChild(fprime, differentiateAST(ASTNode_getChild(f,0),x));
      /* (cos(a))^2 */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_POWER);
      /* cos(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,0),
		      AST_FUNCTION_COS);
      ASTNode_addChild(child2(fprime,1,0),
		       copyAST(ASTNode_getChild(f,0)));
      /* ^2 */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger(child2(fprime,1,1), 2);        
      break;
    case AST_FUNCTION_TANH:
      /* f(x)= tanh(a(x)) => f' = a' / (cosh(a))^2  */
      ASTNode_setType(fprime,AST_DIVIDE);
      /* a' */
      ASTNode_addChild(fprime, differentiateAST(ASTNode_getChild(f,0),x));
      /* (cos(a))^2 */
      ASTNode_addChild(fprime, ASTNode_create());
      ASTNode_setType(ASTNode_getChild(fprime,1), AST_POWER);
      /* cos(a) */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setType(child2(fprime,1,0),
		      AST_FUNCTION_COSH);
      ASTNode_addChild(child2(fprime,1,0),
		       copyAST(ASTNode_getChild(f,0)));
      /* ^2 */
      ASTNode_addChild(ASTNode_getChild(fprime,1), ASTNode_create());
      ASTNode_setInteger(child2(fprime,1,1), 2); 
      break;
    default:
       SolverError_error(WARNING_ERROR_TYPE,
			SOLVER_ERROR_AST_UNKNOWN_FAILURE,
                        "differentiateAST: unknown failure for function type");
      ASTNode_setName(fprime, "differentiation_failed");
      break;
    }
  }
  else if ( ASTNode_isLogical(f) || ASTNode_isRelational(f) ) { /* redundant */
    ASTNode_free(fprime);
    fprime = copyAST(f);
  }
  else if ( ASTNode_isUnknown(fprime) ) {
    SolverError_error(WARNING_ERROR_TYPE,
		      SOLVER_ERROR_AST_UNKNOWN_NODE_TYPE,
		      "differentiateAST: unknown ASTNode type");
    ASTNode_setName(fprime, "differentiation_failed");
  }
  /* return fprime; */
  simple = simplifyAST(fprime);
  ASTNode_free(fprime);
  return simple;

}

/* ------------------------------------------------------------------------ */

/**
  !! experimental function determinantNAST !! NOT TESTED !!
  !! WILL RUN OUT OF MEMORY FOR BIG SYSTEMS !!
  calculates the determinant of the jacobian matrix, is not
  used and can only be activated with hidden
  commandline option -d.
  Doesn't work if expressions get too big for yet unknown
  reasons!??
*/


ASTNode_t *
determinantNAST(ASTNode_t ***A, int N) {

  int k, i, j, l, check;
  ASTNode_t ***B;
  ASTNode_t *det, *tmp, *tmp2, *simple;
  
  det = NULL;
  B = NULL;
  tmp = NULL;

  if ( N == 1 ) {
    return copyAST(A[0][0]);    
  }  

  det = ASTNode_create();

  /* for i = 1 to N */
  for ( k=0; k<N; k++ ) {
    
    check = 0;
    
    if ( ASTNode_isInteger(A[k][0]) ) {
      if ( ASTNode_getInteger(A[k][0]) == 0 ) {
	check = 1;
      }
    }
    if (  ASTNode_isReal(A[k][0]) ) {
      if ( ASTNode_getReal(A[k][0]) == 0.0 ) {
	check = 1;
      }
    }
        
    if ( check < 1 ) { 

      /* B = A, with row i and column 1 deleted */
      B = (ASTNode_t ***) calloc(N-1, sizeof(ASTNode_t **));
      l = 0;
      for ( i=0; i<N-1; i++ ) {
	B[i] = (ASTNode_t **) calloc(N-1, sizeof(ASTNode_t *));
	
	if ( k == i ) {
	  l++;
	}
	for ( j=0; j<N-1; j++ ) {
	  B[i][j] = copyAST(A[l][j+1]); /* ... or A[i+1][l] ?? */
	}	
	l++;
      }

      /*  det = det + (-1)^(i-1)*a(i1) * det(B); but k from 0 to N-1 */

      tmp = ASTNode_create();
      /* ... * ... */
      ASTNode_setType(tmp, AST_TIMES);
      /* (-1)^(i-1)*a(i1) */
      if ( k%2 != 0 ) {
	ASTNode_addChild(tmp, ASTNode_create());
	ASTNode_setType(ASTNode_getChild(tmp, 0), AST_MINUS);
	ASTNode_addChild(ASTNode_getChild(tmp, 0), copyAST(A[k][0]));
      }
      else {
	ASTNode_addChild(tmp, copyAST(A[k][0]));
      }
      /* det(B) */ 
      ASTNode_addChild(tmp, determinantNAST(B, N-1));

      /* det + ... */
      if ( det == NULL ) {
	det = copyAST(tmp);
	ASTNode_free(tmp);
      }
      else {
	tmp2 = ASTNode_create();
	ASTNode_setType(tmp2, AST_PLUS);
	ASTNode_addChild(tmp2, copyAST(det));
	ASTNode_addChild(tmp2, copyAST(tmp));
	ASTNode_free(tmp);
	ASTNode_free(det);
	det = ASTNode_create();
	det = copyAST(tmp2);
	ASTNode_free(tmp2);
      }

      
      for ( i=0; i<N-1; i++ ) {
	for ( j=0; j<N-1; j++ ) {
	  ASTNode_free(B[i][j]);
	}
	free(B[i]);
      }
      free(B);

      /* printf(" +(-1)^(%d-1)*a(%d1)*det(%d) ", k+1, k+1, N-1); */
      
     } 
  }

  simple =  simplifyAST(det);
  ASTNode_free(det);
  return simple;
  
}


/* ------------------------------------------------------------------------ */

int zero(ASTNode_t *f) {
  if ( ASTNode_isReal(f) ) {
    return (ASTNode_getReal(f)==0.0);
  }
  if ( ASTNode_isInteger(f) ) {
    return (ASTNode_getInteger(f)==0);
  }
  return 0;
}

/* ------------------------------------------------------------------------ */

int one(ASTNode_t *f) {
  if ( ASTNode_isReal(f) ) {
    return (ASTNode_getReal(f)==1.0);
  }
  if ( ASTNode_isInteger(f) ) {
    return (ASTNode_getInteger(f)==1);
  }
  return 0;
}

/* ------------------------------------------------------------------------ */

ASTNode_t *
ASTNode_cutRoot(ASTNode_t *old) {
  ASTNode_t *new;
  new = copyAST(ASTNode_getChild(old, 0));
  ASTNode_free(old);
  return new;
}

/* ------------------------------------------------------------------------ */

/* The function simplifyAST(f) takes an AST f,
   simplifies (arithmetic) operations involving 0 and 1
   -0 -> 0
   x+0 -> x, 0+x -> x
   x-0 -> x, 0-x -> -x
   x*0 -> 0, 0*x -> 0, x*1 -> x, 1*x -> x
   0/x -> 0, x/1 -> x
   x^0 -> 1, x^1 -> x, 0^x -> 0, 1^x -> 1
   propagates unary minuses
   - -x -> x
   -x + -y -> -(x+y), -x + y -> y-x,    x + -y -> x-y
   -x - -y -> y-x,    -x - y -> -(x+y), x - -y -> x+y
   -x * -y -> x*y,    -x * y -> -(x*y), x * -y -> -(x*y)
   -x / -y -> x/y,    -x / y -> -(x/y), x / -y -> -(x/y)
   calls evaluateAST(subtree),
   if no variables or user-defined functions occur in the AST subtree,
   calls itself recursively for childnodes,
   and returns a simplified copy of f. */

SBML_ODESOLVER_API ASTNode_t *simplifyAST(ASTNode_t *f) {
  
  int i, childnum;
  int simplify;
  ASTNode_t *simple, *left, *right, *helper;
  ASTNodeType_t type;

  /* new ASTNode */
  simple = ASTNode_create();
  
  type = ASTNode_getType(f);

  /* DISTINCTION OF CASES */

  /* integers, reals */
  if ( ASTNode_isInteger(f) ) {
    ASTNode_setInteger(simple, ASTNode_getInteger(f));
  }
  else if ( ASTNode_isReal(f) ) {
    ASTNode_setReal(simple, ASTNode_getReal(f));
  }
  /* variables */
  else if ( ASTNode_isName(f) ) {
    if ( ASTNode_isSetIndex((ASTNode_t *)f) ) {
      ASTNode_free(simple);
      simple = ASTNode_createIndexName();
      ASTNode_setIndex(simple, ASTNode_getIndex((ASTNode_t *)f));
     } 
    ASTNode_setName(simple, ASTNode_getName(f));
  }
  /* --------------- operators with possible simplifications -------------- */
  /* special operator: unary minus */
  else if ( ASTNode_isUMinus(f) ) {
    left = simplifyAST(ASTNode_getLeftChild(f));
    if ( zero(left) ) {        /* -0 = 0 */
      ASTNode_free(simple);
      simple = left;
    }
    else if ( ASTNode_isUMinus(left) ) {  /* - -x */
      ASTNode_free(simple);
      simple = ASTNode_cutRoot(left);
    }
    else { /* no simplification */
      ASTNode_setType (simple, AST_MINUS);
      ASTNode_addChild(simple, left);
    }
  }
  /* general operators */
  else if ( ASTNode_isOperator(f) || type==AST_FUNCTION_POWER) {
    childnum = ASTNode_getNumChildren(f);  
    /* zero operands: set to neutral element */
    if ( childnum == 0 ) {
      if ( type == AST_PLUS ) 
	ASTNode_setInteger(simple, 0);
      else if ( type == AST_TIMES )
	ASTNode_setInteger(simple, 1);
    }
    /* one operand: set node to operand */
    else if ( childnum == 1 ) {
      ASTNode_free(simple);
      if ( type == AST_PLUS ) 
	simple = simplifyAST(ASTNode_getChild(f, 0));
      else if ( type == AST_TIMES )
	simple = simplifyAST(ASTNode_getChild(f, 0));
    }
    /* >2 operands: recursively decompose
                    into tree with 2 operands */
    else if ( childnum > 2 ) {
      if ( type == AST_PLUS ) 
	ASTNode_setType(simple, AST_PLUS);		  
      else if ( type == AST_TIMES ) 
	ASTNode_setType(simple, AST_TIMES);
      /* copy/simplify left child ... */
      ASTNode_addChild(simple, simplifyAST(ASTNode_getChild(f,0)));
      /* ... and move other child down */
      helper = ASTNode_create();
      ASTNode_setType(helper, type);
      for ( i=1; i<childnum; i++ )
	ASTNode_addChild(helper, simplifyAST(ASTNode_getChild(f,i)));
      ASTNode_addChild(simple, simplifyAST(helper));
      ASTNode_free(helper);
    }
    /* 2 operands: remove 0s and 1s and unary minuses */
    else {    
      left  = simplifyAST(ASTNode_getLeftChild(f));
      right = simplifyAST(ASTNode_getRightChild(f));
      /* default: simplification */
      simplify = 1; /* set flag */
      switch(type) {
	/* binary plus x + y */
      case AST_PLUS:
	if ( zero(right) ) {      /* x+0 = x */
	  ASTNode_free(right);
	  ASTNode_free(simple);
	  simple = left;
	}
	else if ( zero(left) ) {  /* 0+x = x */
	  ASTNode_free(left);
	  ASTNode_free(simple);
	  simple = right;
	}
	else if ( ASTNode_isUMinus(left) && ASTNode_isUMinus(right) ) {
	  ASTNode_setType (simple, AST_MINUS);  /* -x + -y */
	  ASTNode_addChild(simple, ASTNode_create());
	  helper = ASTNode_getChild(simple, 0);
	  ASTNode_setType (helper, AST_PLUS);
	  ASTNode_addChild(helper, ASTNode_cutRoot(left));
	  ASTNode_addChild(helper, ASTNode_cutRoot(right));
	}
	else if ( ASTNode_isUMinus(left) ) {    /* -x + y */
	  ASTNode_setType (simple, AST_MINUS);
	  ASTNode_addChild(simple, right);
	  ASTNode_addChild(simple, ASTNode_cutRoot(left));
	}
	else if ( ASTNode_isUMinus(right) ) {   /* x + -y */
	  ASTNode_setType (simple, AST_MINUS);
	  ASTNode_addChild(simple, left);
	  ASTNode_addChild(simple, ASTNode_cutRoot(right));
	}
	else {
	  simplify = 0;
	}
	break;
	/* binary minus x - y */
      case AST_MINUS:
	if ( zero(right) ) {     /* x-0 = x */
	  ASTNode_free(right);
	  ASTNode_free(simple);
	  simple = left;
	}
	else if ( zero(left) ) { /* 0-x =-x */
	  ASTNode_free(left);
	  ASTNode_setType (simple, type);
	  ASTNode_addChild(simple, right);
	}
	else if ( ASTNode_isUMinus(left) && ASTNode_isUMinus(right) ) { 
	  ASTNode_setType (simple, AST_MINUS);  /* -x - -y */
	  ASTNode_addChild(simple, ASTNode_cutRoot(right));
	  ASTNode_addChild(simple, ASTNode_cutRoot(left));
	}
	else if ( ASTNode_isUMinus(left) ) {    /* -x - y */
	  ASTNode_setType (simple, AST_MINUS); 
	  ASTNode_addChild(simple, ASTNode_create());
	  helper = ASTNode_getChild(simple, 0);
	  ASTNode_setType (helper, AST_PLUS);
	  ASTNode_addChild(helper, ASTNode_cutRoot(left));
	  ASTNode_addChild(helper, right);
	}
	else if ( ASTNode_isUMinus(right) ) {   /* x - -y */
	  ASTNode_setType (simple, AST_PLUS);
	  ASTNode_addChild(simple, left);
	  ASTNode_addChild(simple, ASTNode_cutRoot(right));
	}
	else {
	  simplify = 0;
	}
	break;
	/* binary times x * y */
      case AST_TIMES:
	if ( zero(right) ) {     /* x*0 = 0 */
	  ASTNode_free(left);
	  ASTNode_free(simple);
	  simple = right;
	}
	else if ( zero(left) ) { /* 0*x = 0 */
	  ASTNode_free(right);
	  ASTNode_free(simple);
	  simple = left;
	}
	else if ( one(right) ) { /* x*1 = x */
	  ASTNode_free(right);
	  ASTNode_free(simple);
	  simple = left;
	}
	else if ( one(left) ) {  /* 1*x = x */
	  ASTNode_free(left);
	  ASTNode_free(simple);
	  simple = right;
	}
	else if ( ASTNode_isUMinus(left) && ASTNode_isUMinus(right) ) { 
	  ASTNode_setType (simple, AST_TIMES);  /* -x * -y */
	  ASTNode_addChild(simple, ASTNode_cutRoot(left));
	  ASTNode_addChild(simple, ASTNode_cutRoot(right));
	}
	else if ( ASTNode_isUMinus(left) ) {    /* -x * y */
	  ASTNode_setType (simple, AST_MINUS); 
	  ASTNode_addChild(simple, ASTNode_create());
	  helper = ASTNode_getChild(simple, 0);
	  ASTNode_setType (helper, AST_TIMES);
	  ASTNode_addChild(helper, ASTNode_cutRoot(left));
	  ASTNode_addChild(helper, right);
	}
	else if ( ASTNode_isUMinus(right) ) {   /* x * -y */
	  ASTNode_setType (simple, AST_MINUS); 
	  ASTNode_addChild(simple, ASTNode_create());
	  helper = ASTNode_getChild(simple, 0);
	  ASTNode_setType (helper, AST_TIMES);
	  ASTNode_addChild(helper, left);
	  ASTNode_addChild(helper, ASTNode_cutRoot(right));
	}
	else {
	  simplify = 0;
	}
	break;
	/* binary divide x / y */
      case AST_DIVIDE:
	if ( zero(left) ) {      /* 0/x = 0 */
	  ASTNode_free(right);
	  ASTNode_free(simple);
	  simple = left;
	}
	else if ( one(right) ) { /* x/1 = x */
	  ASTNode_free(right);
	  ASTNode_free(simple);
	  simple = left;
	}
	else if ( ASTNode_isUMinus(left) && ASTNode_isUMinus(right) ) { 
	  ASTNode_setType (simple, AST_DIVIDE); /* -x / -y */
	  ASTNode_addChild(simple, ASTNode_cutRoot(left));
	  ASTNode_addChild(simple, ASTNode_cutRoot(right));
	}
	else if ( ASTNode_isUMinus(left) ) {    /* -x / y */
	  ASTNode_setType (simple, AST_MINUS); 
	  ASTNode_addChild(simple, ASTNode_create());
	  helper = ASTNode_getChild(simple, 0);
	  ASTNode_setType (helper, AST_DIVIDE);
	  ASTNode_addChild(helper, ASTNode_cutRoot(left));
	  ASTNode_addChild(helper, right);
	}
	else if ( ASTNode_isUMinus(right) ) {   /* x / -y */
	  ASTNode_setType (simple, AST_MINUS); 
	  ASTNode_addChild(simple, ASTNode_create());
	  helper = ASTNode_getChild(simple, 0);
	  ASTNode_setType (helper, AST_DIVIDE);
	  ASTNode_addChild(helper, left);
	  ASTNode_addChild(helper, ASTNode_cutRoot(right));
	}
	else {
	  simplify = 0;
	}
	break;
	/* power x^y */
      case AST_POWER:
      case AST_FUNCTION_POWER:
	if ( zero(right) ) {     /* x^0 = 1 */
	  ASTNode_free(left);
	  ASTNode_free(right);
	  ASTNode_setReal(simple, 1.0);      
	}
	else if ( one(right) ) { /* x^1 = x */
	  ASTNode_free(right);
	  ASTNode_free(simple);
	  simple = left;
	}
	else if ( zero(left) ) { /* 0^x = 0 */
	  ASTNode_free(left);
	  ASTNode_free(right);
	  ASTNode_setReal(simple, 0.0);      
	}
	else if ( one(left) ) {  /* 1^x = 1 */
	  ASTNode_free(left);
	  ASTNode_free(right);
	  ASTNode_setReal(simple, 1.0);      
	}
	else {
	  simplify = 0;
	}
	break;    
      default:
	SolverError_error(WARNING_ERROR_TYPE,
			  SOLVER_ERROR_AST_UNKNOWN_FAILURE,
			  "simplifyAST: unknown failure for operator type");
	break;
      }
      /* after all no simplification */
      if (!simplify) {
	ASTNode_setType (simple, type);
	ASTNode_addChild(simple, left);
	ASTNode_addChild(simple, right);
      }
    }
  }
  /* -------------------- cases with no simplifications ------------------- */
  /* constants (leaves) */ 
  /* functions, operators (branches) */
  else {
    ASTNode_setType(simple, type);
    /* user-defined functions: name must be set*/
    if ( ASTNode_getType(f) == AST_FUNCTION ) {
      ASTNode_setName(simple, ASTNode_getName(f));
    }
    for ( i=0; i<ASTNode_getNumChildren(f); i++ ) {
      ASTNode_addChild(simple, simplifyAST(ASTNode_getChild(f,i)));
    }
  }
  
  return (simple);
}

/* End of file */
