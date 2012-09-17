/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  BUI Quang Minh <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef MODELSET_H
#define MODELSET_H

#include "gtrmodel.h"

/**
 * a set of substitution models, used eg for site-specific state frequency model or 
 * partition model with joint branch lengths
 */
class ModelSet : public GTRModel, public vector<GTRModel*>
{

public:
    ModelSet(PhyloTree *tree);
	/**
	 * @return TRUE if this is a site-specific model, FALSE otherwise
	 */
	virtual bool isSiteSpecificModel() { return true; }

	/**
	 * get the size of transition matrix, default is num_states*num_states.
	 * can be changed for e.g. site-specific model
	 */
	virtual int getTransMatrixSize() { return num_states * num_states * size(); }


	/**
		compute the transition probability matrix.
		@param time time between two events
		@param trans_matrix (OUT) the transition matrix between all pairs of states. 
			Assume trans_matrix has size of num_states * num_states.
	*/
	virtual void computeTransMatrix(double time, double *trans_matrix);

	
	/**
	 * wrapper for computing transition matrix times state frequency vector
	 * @param time time between two events
	 * @param trans_matrix (OUT) the transition matrix between all pairs of states.
	 * 	Assume trans_matrix has size of num_states * num_states.
	 */
	virtual void computeTransMatrixFreq(double time, double *trans_matrix);

	
	/**
		compute the transition probability matrix.and the derivative 1 and 2
		@param time time between two events
		@param trans_matrix (OUT) the transition matrix between all pairs of states. 
			Assume trans_matrix has size of num_states * num_states.
		@param trans_derv1 (OUT) the 1st derivative matrix between all pairs of states. 
		@param trans_derv2 (OUT) the 2nd derivative matrix between all pairs of states. 
	*/
	virtual void computeTransDerv(double time, double *trans_matrix, 
		double *trans_derv1, double *trans_derv2);

	/**
		compute the transition probability matrix.and the derivative 1 and 2 times state frequency vector
		@param time time between two events
		@param trans_matrix (OUT) the transition matrix between all pairs of states. 
			Assume trans_matrix has size of num_states * num_states.
		@param trans_derv1 (OUT) the 1st derivative matrix between all pairs of states. 
		@param trans_derv2 (OUT) the 2nd derivative matrix between all pairs of states. 
	*/
	virtual void computeTransDervFreq(double time, double rate_val, double *trans_matrix, 
		double *trans_derv1, double *trans_derv2);


	/**
		return the number of dimensions
	*/
	virtual int getNDim();
	

	/**
		write information
		@param out output stream
	*/
	virtual void writeInfo(ostream &out);
	
    ~ModelSet();
	
protected:
	
	
	/**
		this function is served for the multi-dimension optimization. It should pack the model parameters 
		into a vector that is index from 1 (NOTE: not from 0)
		@param variables (OUT) vector of variables, indexed from 1
	*/
	virtual void setVariables(double *variables);

	/**
		this function is served for the multi-dimension optimization. It should assign the model parameters 
		from a vector of variables that is index from 1 (NOTE: not from 0)
		@param variables vector of variables, indexed from 1
	*/
	virtual void getVariables(double *variables);

	
};

#endif // MODELSET_H
