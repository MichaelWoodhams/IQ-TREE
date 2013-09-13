/*
 * phylotesting.cpp
 *
 *  Created on: Aug 23, 2013
 *      Author: minh
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <iqtree_config.h>
#include "phylotree.h"
#include "iqtree.h"
#include "phylosupertree.h"
#include "phylotesting.h"

#include "gtrmodel.h"
#include "modeldna.h"
#include "myreader.h"
#include "rateheterogeneity.h"
#include "rategamma.h"
#include "rateinvar.h"
#include "rategammainvar.h"
//#include "modeltest_wrapper.h"
#include "modelprotein.h"
#include "modelbin.h"
#include "modelcodon.h"
#include "timeutil.h"


const char* bin_model_names[] = { "JC2", "GTR2" };

const char* dna_model_names[] = { "JC", "F81", "K80", "HKY", "TNe",
		"TN", "K81", "K81u", "TPM2", "TPM2u", "TPM3", "TPM3u", "TIMe", "TIM",
		"TIM2e", "TIM2", "TIM3e", "TIM3", "TVMe", "TVM", "SYM", "GTR" };

const char* dna_model_names_old[] ={"JC", "F81", "K80", "HKY", "TNe",
 	 	 "TN", "K81", "K81u", "TIMe", "TIM", "TVMe", "TVM", "SYM", "GTR"};

const char* dna_model_names_rax[] ={"GTR"};

const char* aa_model_names[] = { "Dayhoff", "mtMAM", "JTT", "WAG",
		"cpREV", "mtREV", "rtREV", "mtART", "mtZOA", "VT", "LG", "DCMut", "PMB",
		"HIVb", "HIVw", "JTTDCMut", "FLU", "Blosum62" };

const char *aa_model_names_old[] = { "Dayhoff", "mtMAM", "JTT", "WAG",
		"cpREV", "mtREV", "rtREV", "mtART", "VT", "LG", "DCMut",
		"HIVb", "HIVw", "Blosum62" };

const char *aa_model_names_rax[] = { "Dayhoff", "mtMAM", "JTT", "WAG",
		"cpREV", "mtREV", "rtREV", "VT", "LG", "DCMut", "Blosum62" };

void computeInformationScores(double tree_lh, int df, int ssize, double &AIC, double &AICc, double &BIC) {
	AIC = -2 * tree_lh + 2 * df;
	AICc = AIC + 2.0 * df * (df + 1) / (ssize - df - 1);
	BIC = -2 * tree_lh + df * log(ssize);
}

double computeInformationScore(double tree_lh, int df, int ssize, ModelTestCriterion mtc) {
	double AIC, AICc, BIC;
	computeInformationScores(tree_lh, df, ssize, AIC, AICc, BIC);
	if (mtc == MTC_AIC)
		return AIC;
	if (mtc == MTC_AICC)
		return AICc;
	if (mtc == MTC_BIC)
		return BIC;
	return 0.0;
}

string criterionName(ModelTestCriterion mtc) {
	if (mtc == MTC_AIC)
		return "AIC";
	if (mtc == MTC_AICC)
		return "AICc";
	if (mtc == MTC_BIC)
		return "BIC";
	return "";
}

void printSiteLh(const char*filename, PhyloTree *tree, double *ptn_lh,
		bool append, const char *linename) {
	int i;
	double *pattern_lh;
	if (!ptn_lh) {
		pattern_lh = new double[tree->getAlnNPattern()];
		tree->computePatternLikelihood(pattern_lh);
	} else
		pattern_lh = ptn_lh;

	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		if (append) {
			out.open(filename, ios::out | ios::app);
		} else {
			out.open(filename);
			out << 1 << " " << tree->getAlnNSite() << endl;
		}
		IntVector pattern_index;
		tree->aln->getSitePatternIndex(pattern_index);
		if (!linename)
			out << "Site_Lh   ";
		else {
			out.width(10);
			out << left << linename;
		}
		for (i = 0; i < tree->getAlnNSite(); i++)
			out << " " << pattern_lh[pattern_index[i]];
		out << endl;
		out.close();
		if (!append)
			cout << "Site log-likelihoods printed to " << filename << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}

	if (!ptn_lh)
		delete[] pattern_lh;
}

/**
 * check if the model file contains correct information
 * @param model_file model file names
 * @param model_name (OUT) vector of model names
 * @param lh_scores (OUT) vector of tree log-likelihoods
 * @param df_vec (OUT) vector of degrees of freedom (or K)
 * @return TRUE if success, FALSE failed.
 */

bool checkModelFile(ifstream &in, bool is_partitioned, vector<ModelInfo> &infos) {
	if (!in.is_open()) return false;
	in.exceptions(ios::badbit);
	string str;
	if (is_partitioned) {
		in >> str;
		if (str != "Charset")
			return false;
	}
	in >> str;
	if (str != "Model")
		return false;
	in >> str;
	if (str != "df")
		return false;
	in >> str;
	if (str != "LnL")
		return false;
	getline(in, str);
	while (!in.eof()) {
		in >> str;
		if (in.eof())
			break;
		ModelInfo info;
		if (is_partitioned) {
			info.set_name = str;
			in >> str;
		}
		info.name = str;
		in >> info.df >> info.logl;
		infos.push_back(info);
		getline(in, str);
		//cout << str << " " << df << " " << logl << endl;
	}
	in.clear();
	return true;
}

bool checkModelFile(string model_file, bool is_partitioned, vector<ModelInfo> &infos) {
	if (!fileExists(model_file))
		return false;
	cout << model_file << " exists, checking this file" << endl;
	ifstream in;
	try {
		in.exceptions(ios::failbit | ios::badbit);
		in.open(model_file.c_str());
		if (!checkModelFile(in, is_partitioned, infos))
			throw false;
		// set the failbit again
		in.exceptions(ios::failbit | ios::badbit);
		in.close();
	} catch (bool ret) {
		in.close();
		return ret;
	} catch (ios::failure) {
		outError("Cannot read file ", model_file);
	}
	return true;
}

/**
 * get the list of model
 * @param nmodels (OUT) number of models
 * @return array of model names
 */
char **getModelList(Params &params, int nstates, int &nmodels) {
	if (nstates == 2) {
		nmodels = sizeof(bin_model_names) / sizeof(char*);
		return (char**)bin_model_names;
	} else if (nstates == 4) {
		if (params.model_set == NULL) {
			nmodels = sizeof(dna_model_names) / sizeof(char*);
			return (char**)dna_model_names;
		} else if (strcmp(params.model_set, "OLD") == 0) {
			nmodels = sizeof(dna_model_names_old) / sizeof(char*);
			return (char**)dna_model_names_old;
		} else if (strcmp(params.model_set, "RAXML") == 0) {
			nmodels = sizeof(dna_model_names_rax) / sizeof(char*);
			return (char**)dna_model_names_rax;
		} else {
			outError("Wrong -mset option");
			nmodels = 0;
			return NULL;
		}
	} else if (nstates == 20) {
		if (params.model_set == NULL) {
			nmodels = sizeof(aa_model_names) / sizeof(char*);
			return (char**)aa_model_names;
		} else if (strcmp(params.model_set, "OLD") == 0) {
			nmodels = sizeof(aa_model_names_old) / sizeof(char*);
			return (char**)aa_model_names_old;
		} else if (strcmp(params.model_set, "RAXML") == 0) {
			nmodels = sizeof(aa_model_names_rax) / sizeof(char*);
			return (char**)aa_model_names_rax;
		} else {
			outError("Wrong -mset option");
			nmodels = 0;
			return NULL;
		}
	} else {
		nmodels = 0;
		return NULL;
	}
}

bool checkPartitionModel(Params &params, PhyloSuperTree *in_tree, vector<ModelInfo> &model_info) {
	PhyloSuperTree::iterator it;
	int i, all_models = 0;
	for (it = in_tree->begin(), i = 0; it != in_tree->end(); it++, i++) {
		int count = 0;
		for (vector<ModelInfo>::iterator mit = model_info.begin(); mit != model_info.end(); mit++)
			if (mit->set_name == in_tree->part_info[i].name)
				count++;
		int nstates = (*it)->aln->num_states;
		int num_models;
		getModelList(params, nstates, num_models);
		if (count != num_models * 4) {
			return false;
		}
		all_models += count;
	}
	return true;
}

void replaceModelInfo(vector<ModelInfo> &model_info, vector<ModelInfo> &new_info) {
	vector<ModelInfo>::iterator first_info = model_info.end(), last_info = model_info.end();
	// scan through models for this partition, assuming the information occurs consecutively
	for (vector<ModelInfo>::iterator mit = model_info.begin(); mit != model_info.end(); mit++)
		if (mit->set_name == new_info.front().set_name) {
			if (first_info == model_info.end()) first_info = mit;
		} else if (first_info != model_info.end()) {
			last_info = mit;
			break;
		}
	if (first_info != model_info.end()) {
		model_info.erase(first_info, last_info);
	}
	model_info.insert(model_info.end(), new_info.begin(), new_info.end());
}

void extractModelInfo(string set_name, vector<ModelInfo> &model_info, vector<ModelInfo> &part_model_info) {
	for (vector<ModelInfo>::iterator mit = model_info.begin(); mit != model_info.end(); mit++)
		if (mit->set_name == set_name)
			part_model_info.push_back(*mit);
}

void mergePartitions(PhyloSuperTree* super_tree, vector<IntVector> &gene_sets, StrVector &model_names) {
	cout << "Merging into " << gene_sets.size() << " partitions..." << endl;
	vector<IntVector>::iterator it;
	SuperAlignment *super_aln = (SuperAlignment*)super_tree->aln;
	vector<PartitionInfo> part_info;
	vector<PhyloTree*> tree_vec;
	for (it = gene_sets.begin(); it != gene_sets.end(); it++) {
		PartitionInfo info;
		info.name = "";
		for (IntVector::iterator i = it->begin(); i != it->end(); i++) {
			if (i != it->begin())
				info.name += "+";
			info.name += super_tree->part_info[*i].name;
		}
		info.model_name = model_names[it-gene_sets.begin()];
		info.aln_file = "";
		info.sequence_type = "";
		info.position_spec = "";
		part_info.push_back(info);
		Alignment *aln = super_aln->concatenateAlignments(*it);
		PhyloTree *tree = super_tree->extractSubtree(*it);
		tree->setAlignment(aln);
		tree_vec.push_back(tree);
	}

	for (PhyloSuperTree::reverse_iterator tit = super_tree->rbegin(); tit != super_tree->rend(); tit++)
		delete (*tit);
	super_tree->clear();
	super_tree->insert(super_tree->end(), tree_vec.begin(), tree_vec.end());
	super_tree->part_info = part_info;

	delete super_tree->aln;
	super_tree->aln = new SuperAlignment(super_tree);
}

/**
 * select models for all partitions
 * @param model_info (IN/OUT) all model information
 * @return total number of parameters
 */
void testPartitionModel(Params &params, PhyloSuperTree* in_tree, vector<ModelInfo> &model_info) {
	int i = 0;
	PhyloSuperTree::iterator it;
	DoubleVector lhvec;
	DoubleVector dfvec;
	double lhsum = 0.0;
	int dfsum = 0;
	int ssize = in_tree->getAlnNSite();
	int nr_model = 1;

	cout << "Selecting models for " << in_tree->size() << " charsets using " << criterionName(params.model_test_criterion) << "..." << endl;
	cout << " No. AIC         AICc        BIC         Charset" << endl;

	for (it = in_tree->begin(), i = 0; it != in_tree->end(); it++, i++) {
		// scan through models for this partition, assuming the information occurs consecutively
		vector<ModelInfo> part_model_info;
		extractModelInfo(in_tree->part_info[i].name, model_info, part_model_info);

		cout.width(4);
		cout << right << nr_model++ << " ";
		string model = testModel(params, (*it), part_model_info, in_tree->part_info[i].name);
		cout << endl;
		in_tree->part_info[i].model_name = model;
		replaceModelInfo(model_info, part_model_info);
		lhvec.push_back(part_model_info[0].logl);
		dfvec.push_back(part_model_info[0].df);
		lhsum += lhvec.back();
		dfsum += dfvec.back();
	}

	if (params.model_name.find("LINK") == string::npos) {
		return;
	}

	/* following implements the greedy algorithm of Lanfear et al. (2012) */
	int part1, part2;
	double inf_score = computeInformationScore(lhsum, dfsum, ssize, params.model_test_criterion);
	cout << "Full partition model " << criterionName(params.model_test_criterion) << " score: " << inf_score << " (lh=" << lhsum << "  df=" << dfsum << ")" << endl;
	SuperAlignment *super_aln = ((SuperAlignment*)in_tree->aln);
	vector<IntVector> gene_sets;
	gene_sets.resize(in_tree->size());
	StrVector model_names;
	model_names.resize(in_tree->size());
	StrVector greedy_model_trees;
	greedy_model_trees.resize(in_tree->size());
	for (i = 0; i < gene_sets.size(); i++) {
		gene_sets[i].push_back(i);
		model_names[i] = in_tree->part_info[i].model_name;
		greedy_model_trees[i] = in_tree->part_info[i].name;
	}
	while (gene_sets.size() >= 2) {
		// stepwise merging charsets
		double new_score = DBL_MAX;
		double opt_lh = 0.0;
		int opt_df = 0;
		int opt_part1 = 0, opt_part2 = 1;
		IntVector opt_merged_set;
		string opt_set_name = "";
		string opt_model_name = "";
		for (part1 = 0; part1 < gene_sets.size()-1; part1++)
			for (part2 = part1+1; part2 < gene_sets.size(); part2++)
			if (super_aln->partitions[gene_sets[part1][0]]->num_states == super_aln->partitions[gene_sets[part2][0]]->num_states) {
				// only merge partitions of the same data type
				IntVector merged_set;
				merged_set.insert(merged_set.end(), gene_sets[part1].begin(), gene_sets[part1].end());
				merged_set.insert(merged_set.end(), gene_sets[part2].begin(), gene_sets[part2].end());
				string set_name = "";
				for (i = 0; i < merged_set.size(); i++) {
					if (i > 0)
						set_name += "+";
					set_name += in_tree->part_info[merged_set[i]].name;
				}
				Alignment *aln = super_aln->concatenateAlignments(merged_set);
				PhyloTree *tree = in_tree->extractSubtree(merged_set);
				tree->setAlignment(aln);
				vector<ModelInfo> part_model_info;
				extractModelInfo(set_name, model_info, part_model_info);

				cout.width(4);
				cout << right << nr_model++ << " ";
				string model = testModel(params, tree, part_model_info, set_name);
				replaceModelInfo(model_info, part_model_info);

				double lhnew = lhsum - lhvec[part1] - lhvec[part2] + part_model_info[0].logl;
				int dfnew = dfsum - dfvec[part1] - dfvec[part2] + part_model_info[0].df;
				double score = computeInformationScore(lhnew, dfnew, ssize, params.model_test_criterion);
				cout << "\t" << score << endl;
				if (score < new_score) {
					new_score = score;
					opt_part1 = part1;
					opt_part2 = part2;
					opt_lh = part_model_info[0].logl;
					opt_df = part_model_info[0].df;
					opt_merged_set = merged_set;
					opt_set_name = set_name;
					opt_model_name = model;
				}
				delete tree;
				delete aln;
			}
		if (new_score >= inf_score) break;
		inf_score = new_score;

		lhsum = lhsum - lhvec[opt_part1] - lhvec[opt_part2] + opt_lh;
		dfsum = dfsum - dfvec[opt_part1] - dfvec[opt_part2] + opt_df;
		cout << "Merging " << opt_set_name << " with " << criterionName(params.model_test_criterion) << " score: " << new_score << " (lh=" << lhsum << "  df=" << dfsum << ")" << endl;
		// change entry opt_part1 to merged one
		gene_sets[opt_part1] = opt_merged_set;
		lhvec[opt_part1] = opt_lh;
		dfvec[opt_part1] = opt_df;
		model_names[opt_part1] = opt_model_name;
		greedy_model_trees[opt_part1] = "(" + greedy_model_trees[opt_part1] + "," + greedy_model_trees[opt_part2] + ")" +
				convertIntToString(in_tree->size()-gene_sets.size()+1) + ":" + convertDoubleToString(inf_score);

		// delete entry opt_part2
		lhvec.erase(lhvec.begin() + opt_part2);
		dfvec.erase(dfvec.begin() + opt_part2);
		gene_sets.erase(gene_sets.begin() + opt_part2);
		model_names.erase(model_names.begin() + opt_part2);
		greedy_model_trees.erase(greedy_model_trees.begin() + opt_part2);
	}

	string final_model_tree;
	if (greedy_model_trees.size() == 1)
		final_model_tree = greedy_model_trees[0];
	else {
		final_model_tree = "(";
		for (i = 0; i < greedy_model_trees.size(); i++) {
			if (i>0)
				final_model_tree += ",";
			final_model_tree += greedy_model_trees[i];
		}
		final_model_tree += ")";
	}

	cout << "BEST-FIT PARTITION MODEL: " << endl;
	cout << "  charpartition " << criterionName(params.model_test_criterion) << " = ";
	for (i = 0; i < gene_sets.size(); i++) {
		if (i > 0)
			cout << ", ";
		cout << model_names[i] << ":";
		for (int j = 0; j < gene_sets[i].size(); j++) {
			cout << " " << in_tree->part_info[gene_sets[i][j]].name;
		}
	}
	cout << ";" << endl;
	cout << "Agglomerative model selection: " << final_model_tree << endl;
	mergePartitions(in_tree, gene_sets, model_names);
}

string testModel(Params &params, PhyloTree* in_tree, vector<ModelInfo> &model_info, string set_name) {
	int nstates = in_tree->aln->num_states;
	if (in_tree->isSuperTree())
		nstates = ((PhyloSuperTree*)in_tree)->front()->aln->num_states;
	if (nstates != 2 && nstates != 4 && nstates != 20)
		outError("Test of best-fit models only works for Binary/DNA/Protein");
	string fmodel_str = params.out_prefix;
	fmodel_str += ".model";
	string sitelh_file = params.out_prefix;
	sitelh_file += ".sitelh";
	in_tree->params = &params;

	int num_models;
	char **model_names = getModelList(params, nstates, num_models);
	int model, rate_type;

	string best_model;
	/* first check the model file */
	bool ok_model_file = false;
	if (!params.print_site_lh) {
		if (set_name == "")
			ok_model_file = checkModelFile(fmodel_str, in_tree->isSuperTree(), model_info);
		else
			ok_model_file = true;
	}
	if (in_tree->isSuperTree()) {
		ok_model_file &= checkPartitionModel(params, (PhyloSuperTree*)in_tree, model_info);
	} else {
		ok_model_file &= (model_info.size() == num_models * 4);
	}
	ofstream fmodel;
	if (ok_model_file) {
		if (set_name == "")
			cout << fmodel_str << " seems to be a correct model file" << endl;
	} else {
		model_info.clear();
		if (set_name == "")
			fmodel.open(fmodel_str.c_str());
		else
			fmodel.open(fmodel_str.c_str(), ios::app);
		if (!fmodel.is_open())
			outError("cannot write to ", fmodel_str);

		if (set_name == "") {
			// print header
			if (in_tree->isSuperTree())
				fmodel << "Charset\t";
			fmodel << "Model\tdf\tLnL";
			if (nstates == 2)
				fmodel << "\t0\t1";
			else if (nstates == 4)
				fmodel << "\tA-C\tA-G\tA-T\tC-G\tC-T\tG-T\tA\tC\tG\tT";
			fmodel << "\talpha\tpinv" << endl;
		}

		fmodel.precision(4);
		fmodel << fixed;
	}

	if (in_tree->isSuperTree()) {
		// select model for each partition
		if (!ok_model_file)
			fmodel.close();
		testPartitionModel(params, (PhyloSuperTree*)in_tree, model_info);
		return "";
	}

	PhyloTree *tree_homo = new PhyloTree();
	tree_homo->optimize_by_newton = params.optimize_by_newton;
	tree_homo->sse = params.SSE;
	tree_homo->copyPhyloTree(in_tree);

	PhyloTree *tree_hetero = new PhyloTree();
	tree_hetero->optimize_by_newton = params.optimize_by_newton;
	tree_hetero->sse = params.SSE;
	tree_hetero->copyPhyloTree(in_tree);

	RateHeterogeneity * rate_class[4];
	rate_class[0] = new RateHeterogeneity();
	rate_class[1] = new RateInvar(-1, NULL);
	rate_class[2] = new RateGamma(params.num_rate_cats, -1, params.gamma_median, NULL);
	rate_class[3] = new RateGammaInvar(params.num_rate_cats, -1, params.gamma_median, -1, params.optimize_gamma_invar_by_bfgs, NULL);
	GTRModel *subst_model = NULL;
	if (nstates == 2)
		subst_model = new ModelBIN("JC2", "", FREQ_UNKNOWN, "", in_tree);
	else if (nstates == 4)
		subst_model = new ModelDNA("JC", "", FREQ_UNKNOWN, "", in_tree);
	else if (nstates == 20)
		subst_model = new ModelProtein("WAG", "", FREQ_UNKNOWN, "", in_tree);
	else if (in_tree->aln->codon_table)
		subst_model = new ModelCodon("GY", "", FREQ_UNKNOWN, "", in_tree);

	assert(subst_model);

	ModelFactory *model_fac = new ModelFactory();

	int ssize = in_tree->aln->getNSite(); // sample size
	if (params.model_test_sample_size)
		ssize = params.model_test_sample_size;
	if (set_name == "") {
		cout << "Testing " << num_models * 4
			<< ((nstates == 2) ? "binary" : ((nstates == 4) ? " DNA" : " protein"))
			<< " models (sample size: " << ssize << ") ..." << endl;
		cout << " No. Model         -LnL         df  AIC          AICc         BIC" << endl;
	}
	if (params.print_site_lh) {
		ofstream sitelh_out(sitelh_file.c_str());
		if (!sitelh_out.is_open())
			outError("Cannot write to file ", sitelh_file);
		sitelh_out << num_models*4 << " " << in_tree->getAlnNSite() << endl;
		sitelh_out.close();
	}

	for (model = 0; model < num_models; model++) {
		for (rate_type = 0; rate_type <= 3; rate_type += 1) {
			// initialize tree
			PhyloTree *tree;
			if (rate_type == 0) {
				tree = tree_homo;
			} else if (rate_type == 1) {
				tree = tree_homo;
			} else if (rate_type == 2) {
				tree = tree_hetero;
			} else {
				tree = tree_hetero;
			}
			// initialize model
			subst_model->init(model_names[model], "", FREQ_UNKNOWN, "");
			subst_model->setTree(tree);
			tree->params = &params;

			tree->setModel(subst_model);
			// initialize rate
			tree->setRate(rate_class[rate_type]);
			rate_class[rate_type]->setTree(tree);

			// initialize model factory
			tree->setModelFactory(model_fac);
			model_fac->model = subst_model;
			model_fac->site_rate = rate_class[rate_type];

			string str;
			str = subst_model->name;
			str += rate_class[rate_type]->name;

			// print some infos
			// clear all likelihood values
			tree->clearAllPartialLH();

			ModelInfo info;

			// optimize model parameters
			info.set_name = set_name;
			info.df = subst_model->getNDim() + rate_class[rate_type]->getNDim() + tree->branchNum;
			info.name = str;
			int model_id = -1;
			for (int i = 0; i < model_info.size(); i++)
				if (info.name == model_info[i].name && info.df == model_info[i].df) {
					model_id = i;
					break;
				}
			if (ok_model_file) {
				// sanity check
				if (model_id < 0)
					outError("Incorrect model file, please delete it and rerun again: ", fmodel_str);
				info.logl = model_info[model_id].logl;
			} else {
				info.logl = tree->getModelFactory()->optimizeParameters(false, false);
				// print information to .model file
				if (set_name != "")
					fmodel << set_name << "\t";
				fmodel << str << "\t" << info.df << "\t" << info.logl;
				if (nstates == 4) {
					int nrates = tree->getModel()->getNumRateEntries();
					double *rate_mat = new double[nrates];
					tree->getModel()->getRateMatrix(rate_mat);
					for (int rate = 0; rate < nrates; rate++)
						fmodel << "\t" << rate_mat[rate];
					delete [] rate_mat;
				}
				if (nstates <= 4) {
					double *freqs = new double[nstates];
					tree->getModel()->getStateFrequency(freqs);
					for (int freq = 0; freq < nstates; freq++)
						fmodel << "\t" << freqs[freq];
					delete [] freqs;
				}
				double alpha = tree->getRate()->getGammaShape();
				fmodel << "\t";
				if (alpha > 0) fmodel << alpha; else fmodel << "NA";
				fmodel << "\t";
				double pinvar = tree->getRate()->getPInvar();
				if (pinvar > 0) fmodel << pinvar << endl; else fmodel << "NA" << endl;
				const char *model_name = (params.print_site_lh) ? str.c_str() : NULL;
				if (params.print_site_lh)
					printSiteLh(sitelh_file.c_str(), tree, NULL, true, model_name);
			}
			computeInformationScores(info.logl, info.df, ssize, info.AIC_score, info.AICc_score, info.BIC_score);
			if (ok_model_file) {
				model_info[model_id] = info;
			} else {
				model_info.push_back(info);
			}
			tree->setModel(NULL);
			tree->setModelFactory(NULL);
			tree->setRate(NULL);

			if (set_name != "") continue;
			cout.width(3);
			cout << right << model*4 + rate_type+1 << "  ";
			cout.width(13);
			cout << left << str << " ";
			cout.precision(3);
			cout << fixed;
			cout.width(12);
			cout << -info.logl << " ";
			cout.width(3);
			cout << info.df << " ";
			cout.width(12);
			cout << info.AIC_score << " ";
			cout.width(12);
			cout << info.AICc_score << " " << info.BIC_score;
			cout << endl;

		}
	}
	//cout.unsetf(ios::fixed);
	int model_aic = 0, model_aicc = 0, model_bic = 0;
	vector<ModelInfo>::iterator it;
	for (it = model_info.begin(), model = 0; it != model_info.end(); it++, model++) {
		if ((*it).AIC_score < model_info[model_aic].AIC_score)
			model_aic = model;
		if ((*it).AICc_score < model_info[model_aicc].AICc_score)
			model_aicc = model;
		if ((*it).BIC_score < model_info[model_bic].BIC_score)
			model_bic = model;
	}
	if (set_name == "") {
		cout << "Akaike Information Criterion:           " << model_info[model_aic].name << endl;
		cout << "Corrected Akaike Information Criterion: " << model_info[model_aicc].name << endl;
		cout << "Bayesian Information Criterion:         " << model_info[model_bic].name << endl;
	} else {
		cout.width(11);
		cout << left << model_info[model_aic].name << " ";
		cout.width(11);
		cout << left << model_info[model_aicc].name << " ";
		cout.width(11);
		cout << left << model_info[model_bic].name << " ";
		cout << set_name;
	}

	/* computing model weights */
	double AIC_sum = 0.0, AICc_sum = 0.0, BIC_sum = 0.0;
	for (it = model_info.begin(); it != model_info.end(); it++) {
		it->AIC_weight = exp(-0.5*(it->AIC_score-model_info[model_aic].AIC_score));
		it->AICc_weight = exp(-0.5*(it->AICc_score-model_info[model_aicc].AICc_score));
		it->BIC_weight = exp(-0.5*(it->BIC_score-model_info[model_bic].BIC_score));
		it->AIC_conf = false;
		it->AICc_conf = false;
		it->BIC_conf = false;
		AIC_sum += it->AIC_weight;
		AICc_sum += it->AICc_weight;
		BIC_sum += it->BIC_weight;
	}
	for (it = model_info.begin(); it != model_info.end(); it++) {
		it->AIC_weight /= AIC_sum;
		it->AICc_weight /= AICc_sum;
		it->BIC_weight /= BIC_sum;
	}

	int *model_rank = new int[num_models*4];
	double *scores = new double[model_info.size()];

	/* compute confidence set for BIC */
    AIC_sum = 0.0;
    AICc_sum = 0.0;
    BIC_sum = 0.0;
	for (model = 0; model < model_info.size(); model++)
		scores[model] = model_info[model].BIC_score;
	sort_index(scores, scores+model_info.size(), model_rank);
	for (model = 0; model < model_info.size(); model++) {
		model_info[model_rank[model]].BIC_conf = true;
		BIC_sum += model_info[model_rank[model]].BIC_weight;
		if (BIC_sum > 0.95) break;
	}
	/* compute confidence set for AIC */
	for (model = 0; model < model_info.size(); model++)
		scores[model] = model_info[model].AIC_score;
	sort_index(scores, scores+model_info.size(), model_rank);
	for (model = 0; model < num_models*4; model++) {
		model_info[model_rank[model]].AIC_conf = true;
		AIC_sum += model_info[model_rank[model]].AIC_weight;
		if (AIC_sum > 0.95) break;
	}

	/* compute confidence set for AICc */
	for (model = 0; model < model_info.size(); model++)
		scores[model] = model_info[model].AICc_score;
	sort_index(scores, scores+model_info.size(), model_rank);
	for (model = 0; model < num_models*4; model++) {
		model_info[model_rank[model]].AICc_conf = true;
		AICc_sum += model_info[model_rank[model]].AICc_weight;
		if (AICc_sum > 0.95) break;
	}

	/* sort models by their scores */
	switch (params.model_test_criterion) {
	case MTC_AIC:
		for (model = 0; model < model_info.size(); model++)
			scores[model] = model_info[model].AIC_score;
		best_model = model_info[model_aic].name;
		break;
	case MTC_AICC:
		for (model = 0; model < model_info.size(); model++)
			scores[model] = model_info[model].AICc_score;
		best_model = model_info[model_aicc].name;
		break;
	case MTC_BIC:
		for (model = 0; model < model_info.size(); model++)
			scores[model] = model_info[model].BIC_score;
		best_model = model_info[model_bic].name;
		break;
	}
	sort_index(scores, scores + model_info.size(), model_rank);

	vector<ModelInfo> sorted_info;
	for (model = 0; model < model_info.size(); model++)
		sorted_info.push_back(model_info[model_rank[model]]);
	model_info = sorted_info;

	delete [] model_rank;
	delete [] scores;

	delete model_fac;
	delete subst_model;
	for (rate_type = 3; rate_type >= 0; rate_type--)
		delete rate_class[rate_type];
	delete tree_hetero;
	delete tree_homo;

	if (!ok_model_file)
		fmodel.close();
	if (set_name == "") {
		cout << "Best-fit model: " << best_model << endl;
	}
	if (params.print_site_lh)
		cout << "Site log-likelihoods per model printed to " << sitelh_file << endl;
	return best_model;
}

int countDistinctTrees(const char *filename, bool rooted, IQTree *tree, IntVector &distinct_ids) {
	StringIntMap treels;
	try {
		ifstream in;
		in.exceptions(ios::failbit | ios::badbit);
		in.open(filename);
		// remove the failbit
		in.exceptions(ios::badbit);
		int tree_id;
		for (tree_id = 0; !in.eof(); tree_id++) {
			tree->freeNode();
			tree->readTree(in, rooted);
			tree->setAlignment(tree->aln);
			tree->setRootNode((char*)tree->aln->getSeqName(0).c_str());
		    StringIntMap::iterator it = treels.end();
		    ostringstream ostr;
		    tree->printTree(ostr, WT_TAXON_ID | WT_SORT_TAXA);
		    it = treels.find(ostr.str());
		    if (it != treels.end()) { // already in treels
		    	distinct_ids.push_back(it->second);
		    } else {
		    	distinct_ids.push_back(-1);
		    	treels[ostr.str()] = tree_id;
		    }
			char ch;
			in.exceptions(ios::goodbit);
			(in) >> ch;
			if (in.eof()) break;
			in.unget();
			in.exceptions(ios::failbit | ios::badbit);

		}
		in.close();
	} catch (ios::failure) {
		outError("Cannot read file ", filename);
	}
	return treels.size();
}

//const double TOL_RELL_SCORE = 0.01;

void evaluateTrees(Params &params, IQTree *tree, vector<TreeInfo> &info, IntVector &distinct_ids)
{
	if (!params.treeset_file)
		return;
	cout << endl;
	//MTreeSet trees(params.treeset_file, params.is_rooted, params.tree_burnin, params.tree_max_count);
	cout << "Reading trees in " << params.treeset_file << " ..." << endl;
	int ntrees = countDistinctTrees(params.treeset_file, params.is_rooted, tree, distinct_ids);
	if (ntrees < distinct_ids.size()) {
		cout << "WARNING: " << distinct_ids.size() << " trees detected but only " << ntrees << " distinct trees will be evaluated" << endl;
	} else {
		cout << ntrees << " distinct trees detected" << endl;
	}
	if (ntrees == 0) return;
	ifstream in(params.treeset_file);

	//if (trees.size() == 1) return;
	string tree_file = params.treeset_file;
	tree_file += ".trees";
	ofstream treeout;
	//if (!params.fixed_branch_length) {
		treeout.open(tree_file.c_str());
	//}
	string score_file = params.treeset_file;
	score_file += ".treelh";
	ofstream scoreout;
	if (params.print_tree_lh)
		scoreout.open(score_file.c_str());
	string site_lh_file = params.treeset_file;
	site_lh_file += ".sitelh";
	if (params.print_site_lh) {
		ofstream site_lh_out(site_lh_file.c_str());
		site_lh_out << ntrees << " " << tree->getAlnNSite() << endl;
		site_lh_out.close();
	}

	double time_start = getCPUTime();

	int *boot_samples = NULL;
	int boot;
	//double *saved_tree_lhs = NULL;
	double *tree_lhs = NULL;
	double *pattern_lh = NULL;
	double *pattern_lhs = NULL;
	double *orig_tree_lh = NULL;
	double *max_lh = NULL;
	double *lhdiff_weights = NULL;
	int nptn = tree->getAlnNPattern();
	if (params.topotest_replicates && ntrees > 1) {
		size_t mem_size = (size_t)params.topotest_replicates*nptn*sizeof(int) +
				ntrees*params.topotest_replicates*sizeof(double) +
				(nptn + ntrees*3 + params.topotest_replicates*2)*sizeof(double) +
				ntrees*sizeof(TreeInfo) +
				params.do_weighted_test*(ntrees * nptn * sizeof(double) + ntrees*ntrees*sizeof(double));
		cout << "Note: " << ((double)mem_size/1024)/1024 << " MB of RAM required!" << endl;
		if (mem_size > getMemorySize()-100000)
			outWarning("The required memory does not fit in RAM!");
		cout << "Creating " << params.topotest_replicates << " bootstrap replicates..." << endl;
		if (!(boot_samples = new int [params.topotest_replicates*nptn]))
			outError(ERR_NO_MEMORY);
		for (boot = 0; boot < params.topotest_replicates; boot++)
			tree->aln->createBootstrapAlignment(boot_samples + (boot*nptn), params.bootstrap_spec);
		//if (!(saved_tree_lhs = new double [ntrees * params.topotest_replicates]))
		//	outError(ERR_NO_MEMORY);
		if (!(tree_lhs = new double [ntrees * params.topotest_replicates]))
			outError(ERR_NO_MEMORY);
		if (params.do_weighted_test) {
			if (!(lhdiff_weights = new double [ntrees * ntrees]))
				outError(ERR_NO_MEMORY);
			if (!(pattern_lhs = new double[ntrees* nptn]))
				outError(ERR_NO_MEMORY);
		}
		if (!(pattern_lh = new double[nptn]))
			outError(ERR_NO_MEMORY);
		if (!(orig_tree_lh = new double[ntrees]))
			outError(ERR_NO_MEMORY);
		if (!(max_lh = new double[params.topotest_replicates]))
			outError(ERR_NO_MEMORY);
	}
	int tree_index, tid, tid2;
	info.resize(ntrees);
	//for (MTreeSet::iterator it = trees.begin(); it != trees.end(); it++, tree_index++) {
	for (tree_index = 0, tid = 0; tree_index < distinct_ids.size(); tree_index++) {

		cout << "Tree " << tree_index + 1;
		if (distinct_ids[tree_index] >= 0) {
			cout << " / identical to tree " << distinct_ids[tree_index]+1 << endl;
			// ignore tree
			char ch;
			do {
				in >> ch;
			} while (!in.eof() && ch != ';');
			continue;
		}
		tree->freeNode();
		tree->readTree(in, params.is_rooted);
		tree->setAlignment(tree->aln);
		tree->initializeAllPartialLh();
		tree->fixNegativeBranch(false);
		if (tree->isSuperTree())
			((PhyloSuperTree*) tree)->mapTrees();
		if (!params.fixed_branch_length) {
			tree->curScore = tree->optimizeAllBranches(100, 0.001);
		} else {
			tree->curScore = tree->computeLikelihood();
		}
		treeout << "[ tree " << tree_index+1 << " lh=" << tree->curScore << " ]";
		tree->printTree(treeout);
		treeout << endl;
		if (params.print_tree_lh)
			scoreout << tree->curScore << endl;

		cout << " / LogL: " << tree->curScore << endl;

		if (pattern_lh) {
			tree->computePatternLikelihood(pattern_lh, &(tree->curScore));
			if (params.do_weighted_test)
				memcpy(pattern_lhs + tid*nptn, pattern_lh, nptn*sizeof(double));
		}
		if (params.print_site_lh) {
			string tree_name = "Tree" + convertIntToString(tree_index+1);
			printSiteLh(site_lh_file.c_str(), tree, pattern_lh, true, tree_name.c_str());
		}
		info[tid].logl = tree->curScore;

		if (!params.topotest_replicates || ntrees <= 1) {
			tid++;
			continue;
		}
		// now compute RELL scores
		orig_tree_lh[tid] = tree->curScore;
		double *tree_lhs_offset = tree_lhs + (tid*params.topotest_replicates);
		for (boot = 0; boot < params.topotest_replicates; boot++) {
			double lh = 0.0;
			int *this_boot_sample = boot_samples + (boot*nptn);
			for (int ptn = 0; ptn < nptn; ptn++)
				lh += pattern_lh[ptn] * this_boot_sample[ptn];
			tree_lhs_offset[boot] = lh;
		}
		tid++;
	}

	assert(tid == ntrees);

	if (params.topotest_replicates && ntrees > 1) {
		double *tree_probs = new double[ntrees];
		memset(tree_probs, 0, ntrees*sizeof(double));
		int *tree_ranks = new int[ntrees];

		/* perform RELL BP method */
		cout << "Performing RELL test..." << endl;
		int *maxtid = new int[params.topotest_replicates];
		double *maxL = new double[params.topotest_replicates];
		int *maxcount = new int[params.topotest_replicates];
		memset(maxtid, 0, params.topotest_replicates*sizeof(int));
		memcpy(maxL, tree_lhs, params.topotest_replicates*sizeof(double));
		for (boot = 0; boot < params.topotest_replicates; boot++)
			maxcount[boot] = 1;
		for (tid = 1; tid < ntrees; tid++) {
			double *tree_lhs_offset = tree_lhs + (tid * params.topotest_replicates);
			for (boot = 0; boot < params.topotest_replicates; boot++)
				if (tree_lhs_offset[boot] > maxL[boot] + params.ufboot_epsilon) {
					maxL[boot] = tree_lhs_offset[boot];
					maxtid[boot] = tid;
					maxcount[boot] = 1;
				} else if (tree_lhs_offset[boot] > maxL[boot] - params.ufboot_epsilon &&
						random_double() <= 1.0/(maxcount[boot]+1)) {
					maxL[boot] = max(maxL[boot],tree_lhs_offset[boot]);
					maxtid[boot] = tid;
					maxcount[boot]++;
				}
		}
		for (boot = 0; boot < params.topotest_replicates; boot++)
			tree_probs[maxtid[boot]] += 1.0;
		for (tid = 0; tid < ntrees; tid++) {
			tree_probs[tid] /= params.topotest_replicates;
			info[tid].rell_confident = false;
			info[tid].rell_bp = tree_probs[tid];
		}
		sort_index(tree_probs, tree_probs + ntrees, tree_ranks);
		double prob_sum = 0.0;
		// obtain the confidence set
		for (tid = ntrees-1; tid >= 0; tid--) {
			info[tree_ranks[tid]].rell_confident = true;
			prob_sum += tree_probs[tree_ranks[tid]];
			if (prob_sum > 0.95) break;
		}

		// sanity check
		for (tid = 0, prob_sum = 0.0; tid < ntrees; tid++)
			prob_sum += tree_probs[tid];
		if (fabs(prob_sum-1.0) > 0.01)
			outError("Internal error: Wrong ", __func__);

		delete [] maxcount;
		delete [] maxL;
		delete [] maxtid;

		/* now do the SH test */
		cout << "Performing KH and SH test..." << endl;
		// SH centering step
		for (boot = 0; boot < params.topotest_replicates; boot++)
			max_lh[boot] = -DBL_MAX;
		double *avg_lh = new double[ntrees];
		for (tid = 0; tid < ntrees; tid++) {
			avg_lh[tid] = 0.0;
			double *tree_lhs_offset = tree_lhs + (tid * params.topotest_replicates);
			for (boot = 0; boot < params.topotest_replicates; boot++)
				avg_lh[tid] += tree_lhs_offset[boot];
			avg_lh[tid] /= params.topotest_replicates;
			for (boot = 0; boot < params.topotest_replicates; boot++) {
				max_lh[boot] = max(max_lh[boot], tree_lhs_offset[boot] - avg_lh[tid]);
			}
		}

		double orig_max_lh = orig_tree_lh[0];
		int orig_max_id = 0;
		double orig_2ndmax_lh = -DBL_MAX;
		int orig_2ndmax_id = -1;
		// find the max tree ID
		for (tid = 1; tid < ntrees; tid++)
			if (orig_max_lh < orig_tree_lh[tid]) {
				orig_max_lh = orig_tree_lh[tid];
				orig_max_id = tid;
			}
		// find the 2nd max tree ID
		for (tid = 0; tid < ntrees; tid++)
			if (tid != orig_max_id && orig_2ndmax_lh < orig_tree_lh[tid]) {
				orig_2ndmax_lh = orig_tree_lh[tid];
				orig_2ndmax_id = tid;
			}


		// SH compute p-value
		for (tid = 0; tid < ntrees; tid++) {
			double *tree_lhs_offset = tree_lhs + (tid * params.topotest_replicates);
			// SH compute original deviation from max_lh
			info[tid].kh_pvalue = 0.0;
			info[tid].sh_pvalue = 0.0;
			int max_id = (tid != orig_max_id) ? orig_max_id : orig_2ndmax_id;
			double orig_diff = orig_tree_lh[max_id] - orig_tree_lh[tid] - avg_lh[tid];
			double *max_kh = tree_lhs + (max_id * params.topotest_replicates);
			for (boot = 0; boot < params.topotest_replicates; boot++) {
				if (max_lh[boot] - tree_lhs_offset[boot] > orig_diff)
					info[tid].sh_pvalue += 1.0;
				//double max_kh_here = max(max_kh[boot]-avg_lh[max_id], tree_lhs_offset[boot]-avg_lh[tid]);
				double max_kh_here = (max_kh[boot]-avg_lh[max_id]);
				if (max_kh_here - tree_lhs_offset[boot] > orig_diff)
					info[tid].kh_pvalue += 1.0;
			}
			info[tid].sh_pvalue /= params.topotest_replicates;
			info[tid].kh_pvalue /= params.topotest_replicates;
		}

		if (params.do_weighted_test) {

			cout << "Computing pairwise logl difference variance ..." << endl;
			/* computing lhdiff_weights as 1/sqrt(lhdiff_variance) */
			for (tid = 0; tid < ntrees; tid++) {
				double *pattern_lh1 = pattern_lhs + (tid * nptn);
				lhdiff_weights[tid*ntrees+tid] = 0.0;
				for (tid2 = tid+1; tid2 < ntrees; tid2++) {
					double lhdiff_variance = tree->computeLogLDiffVariance(pattern_lh1, pattern_lhs + (tid2*nptn));
					lhdiff_weights[tid*ntrees+tid2] = 1.0/sqrt(lhdiff_variance);
					lhdiff_weights[tid2*ntrees+tid] = lhdiff_weights[tid*ntrees+tid2];
				}
			}

			// Weighted KH and SH test
			cout << "Performing WKH and WSH test..." << endl;
			for (tid = 0; tid < ntrees; tid++) {
				double *tree_lhs_offset = tree_lhs + (tid * params.topotest_replicates);
				info[tid].wkh_pvalue = 0.0;
				info[tid].wsh_pvalue = 0.0;
				double worig_diff = -DBL_MAX;
				int max_id = -1;
				for (tid2 = 0; tid2 < ntrees; tid2++)
					if (tid2 != tid) {
						double wdiff = (orig_tree_lh[tid2] - orig_tree_lh[tid])*lhdiff_weights[tid*ntrees+tid2];
						if (wdiff > worig_diff) {
							worig_diff = wdiff;
							max_id = tid2;
						}
					}
				for (boot = 0; boot < params.topotest_replicates; boot++) {
					double wmax_diff = -DBL_MAX;
					for (tid2 = 0; tid2 < ntrees; tid2++)
						if (tid2 != tid)
							wmax_diff = max(wmax_diff,
									(tree_lhs[tid2*params.topotest_replicates+boot] - avg_lh[tid2] -
									tree_lhs_offset[boot] + avg_lh[tid]) * lhdiff_weights[tid*ntrees+tid2]);
					if (wmax_diff > worig_diff)
						info[tid].wsh_pvalue += 1.0;
					wmax_diff = (tree_lhs[max_id*params.topotest_replicates+boot] - avg_lh[max_id] -
							tree_lhs_offset[boot] + avg_lh[tid]);
					if (wmax_diff >  orig_tree_lh[max_id] - orig_tree_lh[tid])
						info[tid].wkh_pvalue += 1.0;
				}
				info[tid].wsh_pvalue /= params.topotest_replicates;
				info[tid].wkh_pvalue /= params.topotest_replicates;
			}
		}
		/* now to ELW - Expected Likelihood Weight method */
		cout << "Performing ELW test..." << endl;

		for (boot = 0; boot < params.topotest_replicates; boot++)
			max_lh[boot] = -DBL_MAX;
		for (tid = 0; tid < ntrees; tid++) {
			double *tree_lhs_offset = tree_lhs + (tid * params.topotest_replicates);
			for (boot = 0; boot < params.topotest_replicates; boot++)
				max_lh[boot] = max(max_lh[boot], tree_lhs_offset[boot]);
		}
		double *sumL = new double[params.topotest_replicates];
		memset(sumL, 0, sizeof(double) * params.topotest_replicates);
		for (tid = 0; tid < ntrees; tid++) {
			double *tree_lhs_offset = tree_lhs + (tid * params.topotest_replicates);
			for (boot = 0; boot < params.topotest_replicates; boot++) {
				tree_lhs_offset[boot] = exp(tree_lhs_offset[boot] - max_lh[boot]);
				sumL[boot] += tree_lhs_offset[boot];
			}
		}
		for (tid = 0; tid < ntrees; tid++) {
			double *tree_lhs_offset = tree_lhs + (tid * params.topotest_replicates);
			tree_probs[tid] = 0.0;
			for (boot = 0; boot < params.topotest_replicates; boot++) {
				tree_probs[tid] += (tree_lhs_offset[boot] / sumL[boot]);
			}
			tree_probs[tid] /= params.topotest_replicates;
			info[tid].elw_confident = false;
			info[tid].elw_value = tree_probs[tid];
		}

		sort_index(tree_probs, tree_probs + ntrees, tree_ranks);
		prob_sum = 0.0;
		// obtain the confidence set
		for (tid = ntrees-1; tid >= 0; tid--) {
			info[tree_ranks[tid]].elw_confident = true;
			prob_sum += tree_probs[tree_ranks[tid]];
			if (prob_sum > 0.95) break;
		}

		// sanity check
		for (tid = 0, prob_sum = 0.0; tid < ntrees; tid++)
			prob_sum += tree_probs[tid];
		if (fabs(prob_sum-1.0) > 0.01)
			outError("Internal error: Wrong ", __func__);
		delete [] sumL;

		delete [] tree_ranks;
		delete [] tree_probs;

	}
	if (max_lh)
		delete [] max_lh;
	if (orig_tree_lh)
		delete [] orig_tree_lh;
	if (pattern_lh)
		delete [] pattern_lh;
	if (pattern_lhs)
		delete [] pattern_lhs;
	if (lhdiff_weights)
		delete [] lhdiff_weights;
	if (tree_lhs)
		delete [] tree_lhs;
	//if (saved_tree_lhs)
	//	delete [] saved_tree_lhs;
	if (boot_samples)
		delete [] boot_samples;

	if (params.print_tree_lh) {
		scoreout.close();
	}

	treeout.close();
	in.close();

	cout << "Time for evaluating all trees: " << getCPUTime() - time_start << " sec." << endl;

}


void evaluateTrees(Params &params, IQTree *tree) {
	vector<TreeInfo> info;
	IntVector distinct_ids;
	evaluateTrees(params, tree, info, distinct_ids);
}
