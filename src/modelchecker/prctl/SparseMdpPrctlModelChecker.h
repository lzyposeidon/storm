/*
 * SparseMdpPrctlModelChecker.h
 *
 *  Created on: 15.02.2013
 *      Author: Christian Dehnert
 */

#ifndef STORM_MODELCHECKER_PRCTL_SPARSEMDPPRCTLMODELCHECKER_H_
#define STORM_MODELCHECKER_PRCTL_SPARSEMDPPRCTLMODELCHECKER_H_

#include "src/modelchecker/prctl/AbstractModelChecker.h"
#include "src/modelchecker/prctl/GmmxxDtmcPrctlModelChecker.h"
#include "src/models/Mdp.h"
#include "src/utility/vector.h"
#include "src/utility/graph.h"

#include <vector>
#include <stack>

namespace storm {
namespace modelchecker {
namespace prctl {

/*!
 * @brief
 * Interface for all model checkers that can verify PRCTL formulae over MDPs represented as a sparse matrix.
 */
template<class Type>
class SparseMdpPrctlModelChecker : public AbstractModelChecker<Type> {

public:
	/*!
	 * Constructs a SparseMdpPrctlModelChecker with the given model.
	 *
	 * @param model The MDP to be checked.
	 */
	explicit SparseMdpPrctlModelChecker(storm::models::Mdp<Type> const& model) : AbstractModelChecker<Type>(model), minimumOperatorStack() {
		// Intentionally left empty.
	}

	/*!
	 * Copy constructs a SparseMdpPrctlModelChecker from the given model checker. In particular, this means that the newly
	 * constructed model checker will have the model of the given model checker as its associated model.
	 */
	explicit SparseMdpPrctlModelChecker(storm::modelchecker::prctl::SparseMdpPrctlModelChecker<Type> const& modelchecker)
		: AbstractModelChecker<Type>(modelchecker),  minimumOperatorStack() {
		// Intentionally left empty.
	}

	/*!
	 * Virtual destructor. Needs to be virtual, because this class has virtual methods.
	 */
	virtual ~SparseMdpPrctlModelChecker() {
		// Intentionally left empty.
	}

	/*!
	 * Returns a constant reference to the MDP associated with this model checker.
	 * @returns A constant reference to the MDP associated with this model checker.
	 */
	storm::models::Mdp<Type> const& getModel() const {
		return AbstractModelChecker<Type>::template getModel<storm::models::Mdp<Type>>();
	}

	/*!
	 * Checks the given formula that is a P/R operator without a bound.
	 *
	 * @param formula The formula to check.
	 * @returns The set of states satisfying the formula represented by a bit vector.
	 */
	virtual std::vector<Type>* checkNoBoundOperator(const storm::property::prctl::AbstractNoBoundOperator<Type>& formula) const {
		// Check if the operator was an non-optimality operator and report an error in that case.
		if (!formula.isOptimalityOperator()) {
			LOG4CPLUS_ERROR(logger, "Formula does not specify neither min nor max optimality, which is not meaningful over nondeterministic models.");
			throw storm::exceptions::InvalidArgumentException() << "Formula does not specify neither min nor max optimality, which is not meaningful over nondeterministic models.";
		}
		minimumOperatorStack.push(formula.isMinimumOperator());
		std::vector<Type>* result = formula.check(*this, false);
		minimumOperatorStack.pop();
		return result;
	}

	/*!
	 * Checks the given formula that is a bounded-until formula.
	 *
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bounds 0 and 1. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bounds 0 and 1.
	 * @returns The probabilities for the given formula to hold on every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact probabilities might not be computed.
	 */
	virtual std::vector<Type>* checkBoundedUntil(const storm::property::prctl::BoundedUntil<Type>& formula, bool qualitative) const {
		// First, we need to compute the states that satisfy the sub-formulas of the until-formula.
		storm::storage::BitVector* leftStates = formula.getLeft().check(*this);
		storm::storage::BitVector* rightStates = formula.getRight().check(*this);
		std::vector<Type>* result = new std::vector<Type>(this->getModel().getNumberOfStates());
        
        // Determine the states that have 0 probability of reaching the target states.
        storm::storage::BitVector statesWithProbabilityGreater0;
        if (this->minimumOperatorStack.top()) {
			statesWithProbabilityGreater0 = storm::utility::graph::performProbGreater0A(this->getModel(), this->getModel().getBackwardTransitions(), *leftStates, *rightStates, true, formula.getBound());
		} else {
			statesWithProbabilityGreater0 = storm::utility::graph::performProbGreater0E(this->getModel(), this->getModel().getBackwardTransitions(), *leftStates, *rightStates, true, formula.getBound());
		}
        
        // Check if we already know the result (i.e. probability 0) for all initial states and
        // don't compute anything in this case.
        if (this->getInitialStates().isDisjointFrom(statesWithProbabilityGreater0)) {
            LOG4CPLUS_INFO(logger, "The probabilities for the initial states were determined in a preprocessing step."
                           << " No exact probabilities were computed.");
            // Set the values for all maybe-states to 0.5 to indicate that their probability values are not 0 (and
            // not necessarily 1).
            storm::utility::vector::setVectorValues(*result, statesWithProbabilityGreater0, Type(0.5));
        } else {
            // In this case we have have to compute the probabilities.
            
            // We can eliminate the rows and columns from the original transition probability matrix that have probability 0.
            storm::storage::SparseMatrix<Type> submatrix = this->getModel().getTransitionMatrix().getSubmatrix(statesWithProbabilityGreater0, this->getModel().getNondeterministicChoiceIndices());
            
            // Get the "new" nondeterministic choice indices for the submatrix.
            std::vector<uint_fast64_t> subNondeterministicChoiceIndices = this->computeNondeterministicChoiceIndicesForConstraint(statesWithProbabilityGreater0);
            
            // Compute the new set of target states in the reduced system.
            storm::storage::BitVector rightStatesInReducedSystem = statesWithProbabilityGreater0 % *rightStates;
            
            // Make all rows absorbing that satisfy the second sub-formula.
            submatrix.makeRowsAbsorbing(rightStatesInReducedSystem, subNondeterministicChoiceIndices);
            
            // Create the vector with which to multiply.
            std::vector<Type> subresult(statesWithProbabilityGreater0.getNumberOfSetBits());
            storm::utility::vector::setVectorValues(subresult, rightStatesInReducedSystem, storm::utility::constGetOne<Type>());
            
            this->performMatrixVectorMultiplication(submatrix, subresult, subNondeterministicChoiceIndices, nullptr, formula.getBound());
            
            // Set the values of the resulting vector accordingly.
            storm::utility::vector::setVectorValues(*result, statesWithProbabilityGreater0, subresult);
            storm::utility::vector::setVectorValues(*result, ~statesWithProbabilityGreater0, storm::utility::constGetZero<Type>());
        }

		// Delete intermediate results and return result.
		delete leftStates;
		delete rightStates;
		return result;
	}

	/*!
	 * Checks the given formula that is a next formula.
	 *
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bounds 0 and 1. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bounds 0 and 1.
	 * @returns The probabilities for the given formula to hold on every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact probabilities might not be computed.
	 */
	virtual std::vector<Type>* checkNext(const storm::property::prctl::Next<Type>& formula, bool qualitative) const {
		// First, we need to compute the states that satisfy the sub-formula of the next-formula.
		storm::storage::BitVector* nextStates = formula.getChild().check(*this);

		// Create the vector with which to multiply and initialize it correctly.
		std::vector<Type>* result = new std::vector<Type>(this->getModel().getNumberOfStates());
		storm::utility::vector::setVectorValues(*result, *nextStates, storm::utility::constGetOne<Type>());

		// Delete obsolete sub-result.
		delete nextStates;

		this->performMatrixVectorMultiplication(this->getModel().getTransitionMatrix(), *result, this->getModel().getNondeterministicChoiceIndices());

		// Return result.
		return result;
	}

	/*!
	 * Checks the given formula that is a bounded-eventually formula.
	 *
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bounds 0 and 1. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bounds 0 and 1.
	 * @returns The probabilities for the given formula to hold on every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact probabilities might not be computed.
	 */
	virtual std::vector<Type>* checkBoundedEventually(const storm::property::prctl::BoundedEventually<Type>& formula, bool qualitative) const {
		// Create equivalent temporary bounded until formula and check it.
		storm::property::prctl::BoundedUntil<Type> temporaryBoundedUntilFormula(new storm::property::prctl::Ap<Type>("true"), formula.getChild().clone(), formula.getBound());
        return this->checkBoundedUntil(temporaryBoundedUntilFormula, qualitative);
	}

	/*!
	 * Checks the given formula that is an eventually formula.
	 *
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bounds 0 and 1. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bounds 0 and 1.
	 * @returns The probabilities for the given formula to hold on every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact probabilities might not be computed.
	 */
	virtual std::vector<Type>* checkEventually(const storm::property::prctl::Eventually<Type>& formula, bool qualitative) const {
		// Create equivalent temporary until formula and check it.
		storm::property::prctl::Until<Type> temporaryUntilFormula(new storm::property::prctl::Ap<Type>("true"), formula.getChild().clone());
		return this->checkUntil(temporaryUntilFormula, qualitative);
	}

	/*!
	 * Checks the given formula that is a globally formula.
	 *
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bounds 0 and 1. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bounds 0 and 1.
	 * @returns The probabilities for the given formula to hold on every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact probabilities might not be computed.
	 */
	virtual std::vector<Type>* checkGlobally(const storm::property::prctl::Globally<Type>& formula, bool qualitative) const {
		// Create "equivalent" temporary eventually formula and check it.
		storm::property::prctl::Eventually<Type> temporaryEventuallyFormula(new storm::property::prctl::Not<Type>(formula.getChild().clone()));
		std::vector<Type>* result = this->checkEventually(temporaryEventuallyFormula, qualitative);

		// Now subtract the resulting vector from the constant one vector to obtain final result.
		storm::utility::vector::subtractFromConstantOneVector(*result);
		return result;
	}

    /*!
	 * Check the given formula that is an until formula.
	 *
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bounds 0 and 1. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bounds 0 and 1.
     * @param scheduler If <code>qualitative</code> is false and this vector is non-null and has as many elements as
     * there are states in the MDP, this vector will represent a scheduler for the model that achieves the probability
     * returned by model checking. To this end, the vector will hold the nondeterministic choice made for each state.
	 * @return The probabilities for the given formula to hold on every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact probabilities might not be computed.
	 */
	virtual std::vector<Type>* checkUntil(const storm::property::prctl::Until<Type>& formula, bool qualitative) const {
        return this->checkUntil(this->minimumOperatorStack.top(), formula, qualitative, nullptr);
    }
    
	/*!
	 * Check the given formula that is an until formula.
	 *
     * @param minimize If set, the probability is minimized and maximized otherwise.
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bounds 0 and 1. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bounds 0 and 1.
     * @param scheduler If <code>qualitative</code> is false and this vector is non-null and has as many elements as
     * there are states in the MDP, this vector will represent a scheduler for the model that achieves the probability
     * returned by model checking. To this end, the vector will hold the nondeterministic choice made for each state.
	 * @return The probabilities for the given formula to hold on every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact probabilities might not be computed.
	 */
	virtual std::vector<Type>* checkUntil(bool minimize, const storm::property::prctl::Until<Type>& formula, bool qualitative, std::vector<uint_fast64_t>* scheduler) const {
		// First, we need to compute the states that satisfy the sub-formulas of the until-formula.
		storm::storage::BitVector* leftStates = formula.getLeft().check(*this);
		storm::storage::BitVector* rightStates = formula.getRight().check(*this);

		// Then, we need to identify the states which have to be taken out of the matrix, i.e.
		// all states that have probability 0 and 1 of satisfying the until-formula.
        std::pair<storm::storage::BitVector, storm::storage::BitVector> statesWithProbability01;
		if (minimize) {
			statesWithProbability01 = storm::utility::graph::performProb01Min(this->getModel(), *leftStates, *rightStates);
		} else {
			statesWithProbability01 = storm::utility::graph::performProb01Max(this->getModel(), *leftStates, *rightStates);
		}
		storm::storage::BitVector statesWithProbability0 = std::move(statesWithProbability01.first);
		storm::storage::BitVector statesWithProbability1 = std::move(statesWithProbability01.second);

		// Delete sub-results that are obsolete now.
		delete leftStates;
		delete rightStates;

		storm::storage::BitVector maybeStates = ~(statesWithProbability0 | statesWithProbability1);
		LOG4CPLUS_INFO(logger, "Found " << statesWithProbability0.getNumberOfSetBits() << " 'no' states.");
		LOG4CPLUS_INFO(logger, "Found " << statesWithProbability1.getNumberOfSetBits() << " 'yes' states.");
		LOG4CPLUS_INFO(logger, "Found " << maybeStates.getNumberOfSetBits() << " 'maybe' states.");

		// Create resulting vector.
		std::vector<Type>* result = new std::vector<Type>(this->getModel().getNumberOfStates());

        // Check whether we need to compute exact probabilities for some states.
        if (this->getInitialStates().isDisjointFrom(maybeStates) || qualitative) {
            if (qualitative) {
                LOG4CPLUS_INFO(logger, "The formula was checked qualitatively. No exact probabilities were computed.");
            } else {
                LOG4CPLUS_INFO(logger, "The probabilities for the initial states were determined in a preprocessing step."
                               << " No exact probabilities were computed.");
            }
            // Set the values for all maybe-states to 0.5 to indicate that their probability values
            // are neither 0 nor 1.
            storm::utility::vector::setVectorValues<Type>(*result, maybeStates, Type(0.5));
        } else {
            // In this case we have have to compute the probabilities.
            
			// First, we can eliminate the rows and columns from the original transition probability matrix for states
			// whose probabilities are already known.
			storm::storage::SparseMatrix<Type> submatrix = this->getModel().getTransitionMatrix().getSubmatrix(maybeStates, this->getModel().getNondeterministicChoiceIndices());

			// Get the "new" nondeterministic choice indices for the submatrix.
			std::vector<uint_fast64_t> subNondeterministicChoiceIndices = this->computeNondeterministicChoiceIndicesForConstraint(maybeStates);

			// Prepare the right-hand side of the equation system. For entry i this corresponds to
			// the accumulated probability of going from state i to some 'yes' state.
			std::vector<Type> b = this->getModel().getTransitionMatrix().getConstrainedRowSumVector(maybeStates, this->getModel().getNondeterministicChoiceIndices(), statesWithProbability1, submatrix.getRowCount());

			// Create vector for results for maybe states.
			std::vector<Type> x = this->getInitialValueIterationValues(submatrix, formula);

			// Solve the corresponding system of equations.
			this->solveEquationSystem(minimize, submatrix, x, b, subNondeterministicChoiceIndices);

			// Set values of resulting vector according to result.
			storm::utility::vector::setVectorValues<Type>(*result, maybeStates, x);
		}

		// Set values of resulting vector that are known exactly.
		storm::utility::vector::setVectorValues<Type>(*result, statesWithProbability0, storm::utility::constGetZero<Type>());
		storm::utility::vector::setVectorValues<Type>(*result, statesWithProbability1, storm::utility::constGetOne<Type>());

		return result;
	}

	/*!
	 * Checks the given formula that is an instantaneous reward formula.
	 *
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bound 0. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bound 0.
	 * @returns The reward values for the given formula for every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact values might not be computed.
	 */
	virtual std::vector<Type>* checkInstantaneousReward(const storm::property::prctl::InstantaneousReward<Type>& formula, bool qualitative) const {
		// Only compute the result if the model has a state-based reward model.
		if (!this->getModel().hasStateRewards()) {
			LOG4CPLUS_ERROR(logger, "Missing (state-based) reward model for formula.");
			throw storm::exceptions::InvalidPropertyException() << "Missing (state-based) reward model for formula.";
		}

		// Initialize result to state rewards of the model.
		std::vector<Type>* result = new std::vector<Type>(this->getModel().getStateRewardVector());

		this->performMatrixVectorMultiplication(this->getModel().getTransitionMatrix(), *result, this->getModel().getNondeterministicChoiceIndices(), nullptr, formula.getBound());

		// Return result.
		return result;
	}

	/*!
	 * Check the given formula that is a cumulative reward formula.
	 *
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bound 0. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bound 0.
	 * @returns The reward values for the given formula for every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact values might not be computed.
	 */
	virtual std::vector<Type>* checkCumulativeReward(const storm::property::prctl::CumulativeReward<Type>& formula, bool qualitative) const {
		// Only compute the result if the model has at least one reward model.
		if (!this->getModel().hasStateRewards() && !this->getModel().hasTransitionRewards()) {
			LOG4CPLUS_ERROR(logger, "Missing reward model for formula.");
			throw storm::exceptions::InvalidPropertyException() << "Missing reward model for formula.";
		}

		// Compute the reward vector to add in each step based on the available reward models.
		std::vector<Type> totalRewardVector;
		if (this->getModel().hasTransitionRewards()) {
			totalRewardVector = this->getModel().getTransitionMatrix().getPointwiseProductRowSumVector(this->getModel().getTransitionRewardMatrix());
			if (this->getModel().hasStateRewards()) {
                storm::utility::vector::addVectorsInPlace(totalRewardVector, this->getModel().getStateRewardVector());
			}
		} else {
			totalRewardVector = std::vector<Type>(this->getModel().getStateRewardVector());
		}

		// Initialize result to either the state rewards of the model or the null vector.
		std::vector<Type>* result = nullptr;
		if (this->getModel().hasStateRewards()) {
			result = new std::vector<Type>(this->getModel().getStateRewardVector());
		} else {
			result = new std::vector<Type>(this->getModel().getNumberOfStates());
		}

		this->performMatrixVectorMultiplication(this->getModel().getTransitionMatrix(), *result, this->getModel().getNondeterministicChoiceIndices(), &totalRewardVector, formula.getBound());

		// Delete temporary variables and return result.
		return result;
	}

    /*!
	 * Checks the given formula that is a reachability reward formula.
	 *
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bound 0. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bound 0.
	 * @return The reward values for the given formula for every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact values might not be computed.
	 */
	virtual std::vector<Type>* checkReachabilityReward(const storm::property::prctl::ReachabilityReward<Type>& formula, bool qualitative) const {
        return this->checkReachabilityReward(this->minimumOperatorStack.top(), formula, qualitative, nullptr);
    }
    
	/*!
	 * Checks the given formula that is a reachability reward formula.
	 *
     * @param minimize If set, the reward is to be minimized and maximized otherwise.
	 * @param formula The formula to check.
	 * @param qualitative A flag indicating whether the formula only needs to be evaluated qualitatively, i.e. if the
	 * results are only compared against the bound 0. If set to true, this will most likely results that are only
	 * qualitatively correct, i.e. do not represent the correct value, but only the correct relation with respect to the
	 * bound 0.
     * @param scheduler If <code>qualitative</code> is false and this vector is non-null and has as many elements as
     * there are states in the MDP, this vector will represent a scheduler for the model that achieves the probability
     * returned by model checking. To this end, the vector will hold the nondeterministic choice made for each state.
	 * @return The reward values for the given formula for every state of the model associated with this model
	 * checker. If the qualitative flag is set, exact values might not be computed.
	 */
	virtual std::vector<Type>* checkReachabilityReward(bool minimize, const storm::property::prctl::ReachabilityReward<Type>& formula, bool qualitative, std::vector<uint_fast64_t>* scheduler) const {
		// Only compute the result if the model has at least one reward model.
		if (!this->getModel().hasStateRewards() && !this->getModel().hasTransitionRewards()) {
			LOG4CPLUS_ERROR(logger, "Missing reward model for formula. Skipping formula");
			throw storm::exceptions::InvalidPropertyException() << "Missing reward model for formula.";
		}

		// Determine the states for which the target predicate holds.
		storm::storage::BitVector* targetStates = formula.getChild().check(*this);

		// Determine which states have a reward of infinity by definition.
		storm::storage::BitVector infinityStates;
		storm::storage::BitVector trueStates(this->getModel().getNumberOfStates(), true);
		if (minimize) {
			infinityStates = std::move(storm::utility::graph::performProb1A(this->getModel(), this->getModel().getBackwardTransitions(), trueStates, *targetStates));
		} else {
			infinityStates = std::move(storm::utility::graph::performProb1E(this->getModel(), this->getModel().getBackwardTransitions(), trueStates, *targetStates));
		}
		infinityStates.complement();
		storm::storage::BitVector maybeStates = ~(*targetStates) & ~infinityStates;
		LOG4CPLUS_INFO(logger, "Found " << infinityStates.getNumberOfSetBits() << " 'infinity' states.");
		LOG4CPLUS_INFO(logger, "Found " << targetStates->getNumberOfSetBits() << " 'target' states.");
		LOG4CPLUS_INFO(logger, "Found " << maybeStates.getNumberOfSetBits() << " 'maybe' states.");

		// Create resulting vector.
		std::vector<Type>* result = new std::vector<Type>(this->getModel().getNumberOfStates());

        // Check whether we need to compute exact rewards for some states.
        if (this->getInitialStates().isDisjointFrom(maybeStates)) {
            LOG4CPLUS_INFO(logger, "The rewards for the initial states were determined in a preprocessing step."
                           << " No exact rewards were computed.");
            // Set the values for all maybe-states to 1 to indicate that their reward values
            // are neither 0 nor infinity.
            storm::utility::vector::setVectorValues<Type>(*result, maybeStates, storm::utility::constGetOne<Type>());
        } else {
            // In this case we have to compute the reward values for the remaining states.

			// We can eliminate the rows and columns from the original transition probability matrix for states
			// whose reward values are already known.
			storm::storage::SparseMatrix<Type> submatrix = this->getModel().getTransitionMatrix().getSubmatrix(maybeStates, this->getModel().getNondeterministicChoiceIndices());

			// Get the "new" nondeterministic choice indices for the submatrix.
			std::vector<uint_fast64_t> subNondeterministicChoiceIndices = this->computeNondeterministicChoiceIndicesForConstraint(maybeStates);

			// Prepare the right-hand side of the equation system. For entry i this corresponds to
			// the accumulated probability of going from state i to some 'yes' state.
			std::vector<Type> b(submatrix.getRowCount());

			if (this->getModel().hasTransitionRewards()) {
				// If a transition-based reward model is available, we initialize the right-hand
				// side to the vector resulting from summing the rows of the pointwise product
				// of the transition probability matrix and the transition reward matrix.
				std::vector<Type> pointwiseProductRowSumVector = this->getModel().getTransitionMatrix().getPointwiseProductRowSumVector(this->getModel().getTransitionRewardMatrix());
				storm::utility::vector::selectVectorValues(b, maybeStates, this->getModel().getNondeterministicChoiceIndices(), pointwiseProductRowSumVector);

				if (this->getModel().hasStateRewards()) {
					// If a state-based reward model is also available, we need to add this vector
					// as well. As the state reward vector contains entries not just for the states
					// that we still consider (i.e. maybeStates), we need to extract these values
					// first.
					std::vector<Type> subStateRewards(b.size());
					storm::utility::vector::selectVectorValuesRepeatedly(subStateRewards, maybeStates, this->getModel().getNondeterministicChoiceIndices(), this->getModel().getStateRewardVector());
					storm::utility::vector::addVectorsInPlace(b, subStateRewards);
				}
			} else {
				// If only a state-based reward model is  available, we take this vector as the
				// right-hand side. As the state reward vector contains entries not just for the
				// states that we still consider (i.e. maybeStates), we need to extract these values
				// first.
				storm::utility::vector::selectVectorValuesRepeatedly(b, maybeStates, this->getModel().getNondeterministicChoiceIndices(), this->getModel().getStateRewardVector());
			}

			// Create vector for results for maybe states.
			std::vector<Type> x = this->getInitialValueIterationValues(submatrix, formula);

			// Solve the corresponding system of equations.
			this->solveEquationSystem(minimize, submatrix, x, b, subNondeterministicChoiceIndices, scheduler);

			// Set values of resulting vector according to result.
			storm::utility::vector::setVectorValues<Type>(*result, maybeStates, x);
		}

		// Set values of resulting vector that are known exactly.
		storm::utility::vector::setVectorValues(*result, *targetStates, storm::utility::constGetZero<Type>());
		storm::utility::vector::setVectorValues(*result, infinityStates, storm::utility::constGetInfinity<Type>());

		// Delete temporary storages and return result.
		delete targetStates;
		return result;
	}

protected:
    /*!
     * Computes the vector of choices that need to be made to minimize/maximize the model checking result for each state.
     *
     * @param minimize If set, all choices are resolved such that the solution value becomes minimal and maximal otherwise.
     * @param nondeterministicResult The model checking result for nondeterministic choices of all states.
     * @param takenChoices The output vector that is to store the taken choices.
     * @param nondeterministicChoiceIndices The assignment of states to their nondeterministic choices in the matrix.
     */
    void computeTakenChoices(bool minimize, std::vector<Type> const& nondeterministicResult, std::vector<uint_fast64_t>& takenChoices, std::vector<uint_fast64_t> const& nondeterministicChoiceIndices) const {
        std::vector<Type> temporaryResult(nondeterministicChoiceIndices.size() - 1);
        if (minimize) {
            storm::utility::vector::reduceVectorMin(nondeterministicResult, temporaryResult, nondeterministicChoiceIndices, &takenChoices);
        } else {
            storm::utility::vector::reduceVectorMax(nondeterministicResult, temporaryResult, nondeterministicChoiceIndices, &takenChoices);
        }
    }
    
	/*!
	 * A stack used for storing whether we are currently computing min or max probabilities or rewards, respectively.
	 * The topmost element is true if and only if we are currently computing minimum probabilities or rewards.
	 */
	mutable std::stack<bool> minimumOperatorStack;

private:
	/*!
	 * Performs (repeated) matrix-vector multiplication with the given parameters, i.e. computes x[i+1] = A*x[i] + b
	 * until x[n], where x[0] = x.
	 *
	 * @param A The matrix that is to be multiplied against the vector.
	 * @param x The initial vector that is to be multiplied against the matrix. This is also the output parameter,
	 * i.e. after the method returns, this vector will contain the computed values.
     * @param nondeterministicChoiceIndices The assignment of states to their rows in the matrix.
	 * @param b If not null, this vector is being added to the result after each matrix-vector multiplication.
	 * @param n Specifies the number of iterations the matrix-vector multiplication is performed.
	 * @returns The result of the repeated matrix-vector multiplication as the content of the parameter vector.
	 */
	virtual void performMatrixVectorMultiplication(storm::storage::SparseMatrix<Type> const& A, std::vector<Type>& x, std::vector<uint_fast64_t> const& nondeterministicChoiceIndices, std::vector<Type>* b = nullptr, uint_fast64_t n = 1) const {
		// Create vector for result of multiplication, which is reduced to the result vector after
		// each multiplication.
		std::vector<Type> multiplyResult(A.getRowCount());

		// Now perform matrix-vector multiplication as long as we meet the bound of the formula.
		for (uint_fast64_t i = 0; i < n; ++i) {
			A.multiplyWithVector(x, multiplyResult);

			// Add b if it is non-null.
			if (b != nullptr) {
				storm::utility::vector::addVectorsInPlace(multiplyResult, *b);
			}

			// Reduce the vector x' by applying min/max for all non-deterministic choices as given by the topmost
			// element of the min/max operator stack.
			if (this->minimumOperatorStack.top()) {
				storm::utility::vector::reduceVectorMin(multiplyResult, x, nondeterministicChoiceIndices);
			} else {
				storm::utility::vector::reduceVectorMax(multiplyResult, x, nondeterministicChoiceIndices);
			}
		}
	}

	/*!
	 * Solves the equation system A*x = b given by the parameters.
	 *
     * @param minimize If set, all choices are resolved such that the solution value becomes minimal and maximal otherwise.
	 * @param A The matrix specifying the coefficients of the linear equations.
	 * @param x The solution vector x. The initial values of x represent a guess of the real values to the solver, but
	 * may be ignored.
	 * @param b The right-hand side of the equation system.
     * @param nondeterministicChoiceIndices The assignment of states to their rows in the matrix.
     * @param takenChoices If not null, this vector will be filled with the nondeterministic choices taken by the states
     * to achieve the solution of the equation system. This assumes that the given vector has at least as many elements
     * as there are states in the MDP.
	 * @returns The solution vector x of the system of linear equations as the content of the parameter x.
	 */
	virtual void solveEquationSystem(bool minimize, storm::storage::SparseMatrix<Type> const& A, std::vector<Type>& x, std::vector<Type> const& b, std::vector<uint_fast64_t> const& nondeterministicChoiceIndices, std::vector<uint_fast64_t>* takenChoices = nullptr) const {
        LOG4CPLUS_INFO(logger, "Starting iterative solver.");
		
        // Get the settings object to customize solving.
		storm::settings::Settings* s = storm::settings::instance();

		// Get relevant user-defined settings for solving the equations.
		double precision = s->get<double>("precision");
		unsigned maxIterations = s->get<unsigned>("maxiter");
		bool relative = s->get<bool>("relative");

		// Set up the environment for the power method.
		std::vector<Type> multiplyResult(A.getRowCount());
		std::vector<Type>* currentX = &x;
		std::vector<Type>* newX = new std::vector<Type>(x.size());
		std::vector<Type>* swap = nullptr;
		uint_fast64_t iterations = 0;
		bool converged = false;

		// Proceed with the iterations as long as the method did not converge or reach the
		// user-specified maximum number of iterations.
		while (!converged && iterations < maxIterations) {
			// Compute x' = A*x + b.
			A.multiplyWithVector(*currentX, multiplyResult);
			storm::utility::vector::addVectorsInPlace(multiplyResult, b);

			// Reduce the vector x' by applying min/max for all non-deterministic choices as given by the topmost
			// element of the min/max operator stack.
			if (minimize) {
				storm::utility::vector::reduceVectorMin(multiplyResult, *newX, nondeterministicChoiceIndices);
			} else {
				storm::utility::vector::reduceVectorMax(multiplyResult, *newX, nondeterministicChoiceIndices);
			}

			// Determine whether the method converged.
			converged = storm::utility::vector::equalModuloPrecision(*currentX, *newX, precision, relative);

			// Update environment variables.
			swap = currentX;
			currentX = newX;
			newX = swap;
			++iterations;
		}
        
        // If we were requested to record the taken choices, we have to construct the vector now.
        if (takenChoices != nullptr) {
            this->computeTakenChoices(minimize, multiplyResult, *takenChoices, nondeterministicChoiceIndices);
        }

		// If we performed an odd number of iterations, we need to swap the x and currentX, because the newest result
		// is currently stored in currentX, but x is the output vector.
		if (iterations % 2 == 1) {
			std::swap(x, *currentX);
			delete currentX;
		} else {
			delete newX;
		}

		// Check if the solver converged and issue a warning otherwise.
		if (converged) {
			LOG4CPLUS_INFO(logger, "Iterative solver converged after " << iterations << " iterations.");
		} else {
			LOG4CPLUS_WARN(logger, "Iterative solver did not converge.");
		}
	}
    
	/*!
	 * Computes the nondeterministic choice indices vector resulting from reducing the full system to the states given
	 * by the parameter constraint.
	 *
	 * @param constraint A bit vector specifying which states are kept.
	 * @returns A vector of the nondeterministic choice indices of the subsystem induced by the given constraint.
	 */
	std::vector<uint_fast64_t> computeNondeterministicChoiceIndicesForConstraint(storm::storage::BitVector const& constraint) const {
		// First, get a reference to the full nondeterministic choice indices.
		std::vector<uint_fast64_t> const& nondeterministicChoiceIndices = this->getModel().getNondeterministicChoiceIndices();

		// Reserve the known amount of slots for the resulting vector.
		std::vector<uint_fast64_t> subNondeterministicChoiceIndices(constraint.getNumberOfSetBits() + 1);
		uint_fast64_t currentRowCount = 0;
		uint_fast64_t currentIndexCount = 1;

		// Set the first element as this will clearly begin at offset 0.
		subNondeterministicChoiceIndices[0] = 0;

		// Loop over all states that need to be kept and copy the relative indices of the nondeterministic choices over
		// to the resulting vector.
		for (auto index : constraint) {
			subNondeterministicChoiceIndices[currentIndexCount] = currentRowCount + nondeterministicChoiceIndices[index + 1] - nondeterministicChoiceIndices[index];
			currentRowCount += nondeterministicChoiceIndices[index + 1] - nondeterministicChoiceIndices[index];
			++currentIndexCount;
		}

		// Put a sentinel element at the end.
		subNondeterministicChoiceIndices[constraint.getNumberOfSetBits()] = currentRowCount;

		return subNondeterministicChoiceIndices;
	}
    
    /*!
     * Retrieves the values to be used as the initial values for the value iteration techniques.
     *
     * @param submatrix The matrix that will be used for value iteration later.
     */
    std::vector<Type> getInitialValueIterationValues(storm::storage::SparseMatrix<Type> const& submatrix, storm::property::prctl::AbstractPathFormula<Type> const& formula) const {
        // std::vector<uint_fast64_t> scheduler(submatrix.getColumnCount());
        std::vector<uint_fast64_t> scheduler(this->getModel().getNumberOfStates());
        
        storm::models::Dtmc<Type> dtmc(this->getModel().getTransitionMatrix().getSubmatrix(scheduler, this->getModel().getNondeterministicChoiceIndices()),
                                       storm::models::AtomicPropositionsLabeling(this->getModel().getStateLabeling()),
                                       this->getModel().hasStateRewards() ? boost::optional<std::vector<Type>>(this->getModel().getStateRewardVector()) : boost::optional<std::vector<Type>>(),
                                       this->getModel().hasTransitionRewards() ? boost::optional<storm::storage::SparseMatrix<Type>>(this->getModel().getTransitionRewardMatrix().getSubmatrix(scheduler, this->getModel().getNondeterministicChoiceIndices(), false)) : boost::optional<storm::storage::SparseMatrix<Type>>());
        
        storm::modelchecker::prctl::GmmxxDtmcPrctlModelChecker<Type> modelChecker(dtmc);
        
        std::vector<Type>* modelCheckingResult = formula.check(modelChecker, false);
        std::vector<Type> result(std::move(*modelCheckingResult));
        delete modelCheckingResult;
        
        return result;
    }
    
};

} // namespace prctl
} // namespace modelchecker
} // namespace storm

#endif /* STORM_MODELCHECKER_PRCTL_SPARSEMDPPRCTLMODELCHECKER_H_ */
