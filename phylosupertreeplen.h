/*
 * phylosupertreeplen.h
 *
 *  Created on: Aug 5, 2013
 *      Author: olga
 */

#ifndef PHYLOSUPERTREEPLEN_H_
#define PHYLOSUPERTREEPLEN_H_

#include "phylosupertree.h"
#include "model/partitionmodel.h"
#include "superalignmentpairwise.h"


/**
 * this is to classify the cases which happen on the subtree
 *
 *  NNI_NONE_EPSILON: all 5 branches have images on subtree, this corresponds to change in subtree topology
 * 					  2 partial_lh vectors for -nni1 or 6 partial_lh vectors for -nni5 options
 *  NNI_ONE_EPSILON:  only one of the 5 branches has no image on subtree, this does not change subtree topology, but changes branch length of subtrees
 * 					  we need to allocate partial likelihood memory (1 partial_lh vectors for -nni1 option or 3 partial_lh for -nni5 option)
 * 	NNI_TWO_EPSILON:  two branches (on different sides of central branch) have no images, here after the NNI swap,
 * 					  the image of central branch either does not change or is equal to epsilon (then we decrease the branch length)
 * 					  and no allocation of partial_lh is needed
 * 	NNI_THREE_EPSILON: central and two adjacent edges have no images: after the NNI swap, central branch will have image and we need to relink it
 * 					no allocation of partial_lh is needed
 *  NNI_MANY_EPSILON: more than 3 branches have no images on subtree: nothing changes in subtree and no recomputation of partial likelihood are required
 */
enum NNIType {NNI_NO_EPSILON, NNI_ONE_EPSILON, NNI_TWO_EPSILON, NNI_THREE_EPSILON, NNI_MANY_EPSILON};


/**
Edge lengths in subtrees are proportional to edge lengths in a supertree.

	@author Olga Chernomor <olga.chernomor@univie.ac.at>
*/

class PhyloSuperTreePlen;

// Auxiliary classes ====================================================================================
// ======================================================================================================
class SuperAlignmentPairwisePlen : public SuperAlignmentPairwise {

	public:

		/**
		    constructor
		 */

	    SuperAlignmentPairwisePlen();

		/**
			construct the pairwise alignment from two sequences of a multiple alignment
			@param aln input multiple alignment
			@param seq_id1 ID of the first sequence
			@param seq_id2 ID of the second sequence
		*/
		SuperAlignmentPairwisePlen(PhyloSuperTreePlen *atree, int seq1, int seq2);

	    ~SuperAlignmentPairwisePlen();

		/**
			compute the likelihood for a distance between two sequences. Used for the ML optimization of the distance.
			@param value x-value of the function
			@return log-likelihood
		*/
		virtual double computeFunction(double value);

		/**
			This function calculate f(value), first derivative f'(value) and 2nd derivative f''(value).
			used by Newton raphson method to minimize the function.
			@param value x-value of the function
			@param df (OUT) first derivative
			@param ddf (OUT) second derivative
			@return f(value) of function f you want to minimize
		*/
		virtual void computeFuncDerv(double value, double &df, double &ddf);

		/**
			partition information
		*/
		vector<PartitionInfo>* part_info;

};
// ======================================================================================================
class PartitionModelPlen : public PartitionModel
{
public:
    PartitionModelPlen();
	/**
		constructor
		create partition model with possible rate heterogeneity. Create proper class objects
		for two variables: model and site_rate. It takes the following field of params into account:
			model_name, num_rate_cats, freq_type, store_trans_matrix
		@param params program parameters
		@param tree associated phylogenetic super-tree
	*/
	PartitionModelPlen(Params &params, PhyloSuperTreePlen *tree, ModelsBlock *models_block);

    ~PartitionModelPlen();

    /**
        save object into the checkpoint
    */
    virtual void saveCheckpoint();

    /**
        restore object from the checkpoint
    */
    virtual void restoreCheckpoint();

    /**
     * @return #parameters of the model + # branches
     */
    virtual int getNParameters();
    virtual int getNDim();

	/**
		write information
		@param out output stream
	*/
	virtual void writeInfo(ostream &out);

	/**
		optimize model parameters and tree branch lengths
		@param fixed_len TRUE to fix branch lengths, default is false
		@return the best likelihood
	*/
	virtual double optimizeParameters(bool fixed_len = false, bool write_info = true,
                                      double logl_epsilon = 0.1, double gradient_epsilon = 0.001);


	/**
	 *  optimize model parameters and tree branch lengths for the +I+G model
	 *  using restart strategy.
	 * 	@param fixed_len TRUE to fix branch lengths, default is false
	 *	@return the best likelihood
	 */
	virtual double optimizeParametersGammaInvar(bool fixed_len = false, bool write_info = true, double logl_epsilon = 0.1, double gradient_epsilon = 0.001);

	double optimizeGeneRate(double tol);

//	virtual double targetFunk(double x[]);
//	virtual void getVariables(double *variables);
//	virtual void setVariables(double *variables);

    /** partition ID currently under optimization of of its rate */
//    int optimizing_part;

    /**
        compute the likelihood for a partition under rate optimization (optimizing_rate).
        Used for the ML optimization of gene rate
        @param value x-value of the function
        @return log-likelihood
    */
//    virtual double computeFunction(double value);


};

// ======================================================================================================
// ======================================================================================================

class PhyloSuperTreePlen : public PhyloSuperTree {

public:
	/**
		constructors
	*/
	PhyloSuperTreePlen();
	PhyloSuperTreePlen(Params &params);
	PhyloSuperTreePlen(SuperAlignment *alignment, PhyloSuperTree *super_tree);

	~PhyloSuperTreePlen();

    /**
        save object into the checkpoint
    */
    virtual void saveCheckpoint();

    /**
        restore object from the checkpoint
    */
    virtual void restoreCheckpoint();

    /**
            Read the tree saved with Taxon Names and branch lengths.
            @param tree_string tree string to read from
     */
    virtual void readTreeString(const string &tree_string);

    /**
     * Return the tree string containing taxon names and branch lengths
     * @return tree string
     */
    virtual string getTreeString();


    /**
            compute the distance between 2 sequences.
            @param seq1 index of sequence 1
            @param seq2 index of sequence 2
            @param initial_dist initial distance
            @return distance between seq1 and seq2
     */

    virtual double computeDist(int seq1, int seq2, double initial_dist, double &var);

	/**
		create sub-trees T|Y_1,...,T|Y_k of the current super-tree T
		and map F={f_1,...,f_k} the edges of supertree T to edges of subtrees T|Y_i
	*/
	virtual void mapTrees();

	/**
	 * Given current supertree T and subtrees T|Y_1,...,T|Y_k, build all maps f_1,...,f_k
	 */
	virtual void linkTrees();

    /**
            initialize partial_lh vector of all PhyloNeighbors, allocating central_partial_lh
     */
    virtual void initializeAllPartialLh();

    /**
            initialize partial_lh vector of all PhyloNeighbors, allocating central_partial_lh
            @param node the current node
            @param dad dad of the node, used to direct the search
            @param index the index
     */
    virtual void initializeAllPartialLh(int &index, int &indexlh, PhyloNode *node = NULL, PhyloNode *dad = NULL);

    void initializeAllPartialLh(double* &lh_addr, UBYTE* &scale_addr, UINT* &pars_addr, PhyloNode *node = NULL, PhyloNode *dad = NULL);

    /**
            de-allocate central_partial_lh
     */
    virtual void deleteAllPartialLh();

	/**
	 * @return the type of NNI around node1-node2 for partition part
	 */
	void getNNIType(PhyloNode *node1, PhyloNode *node2, vector<NNIType> &nni_type);

    /**
            Inherited from Optimization class.
            This function calculate f(value), first derivative f'(value) and 2nd derivative f''(value).
            used by Newton raphson method to minimize the function.
            @param value current branch length
            @param df (OUT) first derivative
            @param ddf (OUT) second derivative
            @return negative of likelihood (for minimization)
     */
    virtual void computeFuncDerv(double value, double &df, double &ddf);

    /**
            inherited from Optimization class, to return to likelihood of the tree
            when the current branch length is set to value
            @param value current branch length
            @return negative of likelihood (for minimization)
     */
    virtual double computeFunction(double value);

    /**
            compute tree likelihood on a branch. used to optimize branch length
            @param dad_branch the branch leading to the subtree
            @param dad its dad, used to direct the tranversal
            @return tree likelihood
     */
    virtual double computeLikelihoodBranch(PhyloNeighbor *dad_branch, PhyloNode *dad);

    /**
            compute tree likelihood on a branch given buffer (theta_all), used after optimizing branch length
            @return tree likelihood
     */

    virtual double computeLikelihoodFromBuffer();

    /**
            optimize all branch lengths of all subtrees, then compute branch lengths
            of supertree as weighted average over all subtrees
            @param iterations number of iterations to loop through all branches
            @return the likelihood of the tree
     */
    virtual double optimizeAllBranches(int my_iterations = 100, double tolerance = TOL_LIKELIHOOD, int maxNRStep = 100);

    /**
            optimize one branch length by ML by optimizing all mapped branches of subtrees
            @param node1 1st end node of the branch
            @param node2 2nd end node of the branch
            @param clearLH true to clear the partial likelihood, otherwise false
            @return likelihood score
     */
    virtual void optimizeOneBranch(PhyloNode *node1, PhyloNode *node2, bool clearLH = true, int maxNRStep = 100);

    /**
            search the best swap for a branch
            @return NNIMove The best Move/Swap
            @param cur_score the current score of the tree before the swaps
            @param node1 1 of the 2 nodes on the branch
            @param node2 1 of the 2 nodes on the branch
     */
    virtual NNIMove getBestNNIForBran(PhyloNode *node1, PhyloNode *node2, NNIMove *nniMoves = NULL);


    /**
            Do an NNI on the supertree and synchronize all subtrees respectively
            @param move the single NNI
     */
    virtual void doNNI(NNIMove &move, bool clearLH = true);
    /**
            apply nni2apply NNIs from the non-conflicting NNI list
            @param nni2apply number of NNIs to apply from the list
            @param changeBran whether or not the computed branch lengths should be applied
     */
    virtual void doNNIs(int nni2apply, bool changeBran = true);

    /**
     *   Apply 5 new branch lengths stored in the NNI move
     *   @param nnimove the NNI move currently in consideration
     */
    virtual void changeNNIBrans(NNIMove nnimove);

    /**
            This is for ML. try to swap the tree with nearest neigbor interchange at the branch connecting node1-node2.
            If a swap shows better score, return the swapped tree and the score.
            @param cur_score current likelihood score
            @param node1 1st end node of the branch
            @param node2 2nd end node of the branch
            @param nni_param (OUT) if not NULL: swapping information returned
            @return the likelihood of the tree
     */
    virtual double swapNNIBranch(double cur_score, PhyloNode *node1, PhyloNode *node2, SwapNNIParam *nni_param = NULL, NNIMove *nniMoves = NULL);

    /**
     *	used in swapNNIBranch to update link_neighbors of other SuperNeighbors that point to the same branch on SubTree as (node,dad)
     *	@param saved_link_dad_nei   pointer to link_neighbor dad_nei
     */
    void linkCheck(int part, Node* node, Node* dad, PhyloNeighbor* saved_link_dad_nei);
    void linkCheckRe(int part, Node* node, Node* dad, PhyloNeighbor* saved_link_dad_nei,PhyloNeighbor* saved_link_node_nei);

	/**
		compute the weighted average of branch lengths over partitions
	*/
	virtual void computeBranchLengths();

	bool checkBranchLen();
	void mapBranchLen();
	void mapBranchLen(int part);
	virtual void printMapInfo();

//	virtual void restoreAllBrans(PhyloNode *node, PhyloNode *dad);

	/**
	 * initialize partition information for super tree
	*/
	virtual void initPartitionInfo();

	void printNNIcasesNUM();

    /**
     * 		indicates whether partition rates are fixed or not
     */

    bool fixed_rates;

    /*
     * 1 - # of is_nni on subtree
     * 2 - # of relink branch to an empty one
     * 3 - # of empty to empty
     * 4 - # of relink branch to a  new one (50% saving on these cases compared to the previous implementation)
     * 5 - # of relink branch to an old one + relink empty to some branch (100% saving on these cases)
     */
    int allNNIcases_computed[5];

    /**
            Neighbor-joining/parsimony tree might contain negative branch length. This
            function will fix this.
            @param fixed_length fixed branch length to set to negative branch lengths
            @param node the current node
            @param dad dad of the node, used to direct the search
            @return The number of branches that have no/negative length
     */
    virtual int fixNegativeBranch(bool force = false, Node *node = NULL, Node *dad = NULL);

protected:
	vector<uint64_t> partial_lh_entries, scale_num_entries, partial_pars_entries, block_size, scale_block_size;

};



#endif /* PHYLOSUPERTREEPLEN_H_ */
