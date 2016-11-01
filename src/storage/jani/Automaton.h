#pragma once

#include <string>
#include <cstdint>
#include <unordered_map>
#include <boost/container/flat_set.hpp>

#include "src/storage/jani/VariableSet.h"
#include "src/storage/jani/Edge.h"
#include "src/storage/jani/Location.h"

namespace storm {
    namespace jani {

        class Automaton;
        
        namespace detail {
            class Edges {
            public:
                typedef std::vector<Edge>::iterator iterator;
                typedef std::vector<Edge>::const_iterator const_iterator;
                
                Edges(iterator it, iterator ite);
                
                /*!
                 * Retrieves an iterator to the edges.
                 */
                iterator begin() const;
                
                /*!
                 * Retrieves an end iterator to the edges.
                 */
                iterator end() const;
                
                /*!
                 * Determines whether this set of edges is empty.
                 */
                bool empty() const;
                
                /*!
                 * Retrieves the number of edges.
                 */
                std::size_t size() const;
                
            private:
                iterator it;
                iterator ite;
            };
            
            class ConstEdges {
            public:
                typedef std::vector<Edge>::iterator iterator;
                typedef std::vector<Edge>::const_iterator const_iterator;
                
                ConstEdges(const_iterator it, const_iterator ite);
                
                /*!
                 * Retrieves an iterator to the edges.
                 */
                const_iterator begin() const;
                
                /*!
                 * Retrieves an end iterator to the edges.
                 */
                const_iterator end() const;

                /*!
                 * Determines whether this set of edges is empty.
                 */
                bool empty() const;

                /*!
                 * Retrieves the number of edges.
                 */
                std::size_t size() const;
                
            private:
                const_iterator it;
                const_iterator ite;
            };
        }
        
        class Model;
        
        class Automaton {
        public:
            friend class detail::Edges;
            friend class detail::ConstEdges;
            
            typedef detail::Edges Edges;
            typedef detail::ConstEdges ConstEdges;
            
            /*!
             * Creates an empty automaton.
             */
            Automaton(std::string const& name);
            
            /*!
             * Retrieves the name of the automaton.
             */
            std::string const& getName() const;
            
            /*!
             * Adds the given variable to this automaton
             */
            Variable const& addVariable(Variable const& variable);

            /*!
             * Adds the given Boolean variable to this automaton.
             */
            BooleanVariable const& addVariable(BooleanVariable const& variable);
            
            /*!
             * Adds the given bounded integer variable to this automaton.
             */
            BoundedIntegerVariable const& addVariable(BoundedIntegerVariable const& variable);
            
            /*!
             * Adds the given unbounded integer variable to this automaton.
             */
            UnboundedIntegerVariable const& addVariable(UnboundedIntegerVariable const& variable);

            /*!
             * Adds the given real variable to this automaton.
             */
            RealVariable const& addVariable(RealVariable const& variable);

            /*!
             * Retrieves the variables of this automaton.
             */
            VariableSet& getVariables();

            /*!
             * Retrieves the variables of this automaton.
             */
            VariableSet const& getVariables() const;
            
            /*!
             * Retrieves whether this automaton has a transient variable.
             */
            bool hasTransientVariable() const;
            
            /*!
             * Retrieves whether the automaton has a location with the given name.
             */
            bool hasLocation(std::string const& name) const;

            /*!
             * Get location id for a location with a given name.
             * Yields undefined behaviour if no such location exists;
             *
             * @name the name of the location
             */
            uint64_t getLocationIndex(std::string const& name) const;
            
            /*!
             * Retrieves the locations of the automaton.
             */
            std::vector<Location> const& getLocations() const;

            /*!
             * Retrieves the locations of the automaton.
             */
            std::vector<Location>& getLocations();

            /*!
             * Retrieves the location with the given index.
             */
            Location const& getLocation(uint64_t index) const;
            
            /*!
             * Retrieves the location with the given index.
             */
            Location& getLocation(uint64_t index);
            
            /*!
             * Adds the given location to the automaton.
             */
            uint64_t addLocation(Location const& location);
            
            /*!
             * Adds the location with the given name to the initial locations.
             */
            void addInitialLocation(std::string const& name);

            /*!
             * Adds the location with the given index to the initial locations.
             */
            void addInitialLocation(uint64_t index);
            
            /*!
             * Retrieves the indices of the initial locations.
             */
            std::set<uint64_t> const& getInitialLocationIndices() const;

            /*!
             * Builds a map from ID to Location Name.
             */
            std::map<uint64_t, std::string> buildIdToLocationNameMap() const;
            
            /*!
             * Retrieves the edges of the location with the given name.
             */
            Edges getEdgesFromLocation(std::string const& name);
            
            /*!
             * Retrieves the edges of the location with the given index.
             */
            Edges getEdgesFromLocation(uint64_t index);
            
            /*!
             * Retrieves the edges of the location with the given name.
             */
            ConstEdges getEdgesFromLocation(std::string const& name) const;

            /*!
             * Retrieves the edges of the location with the given index.
             */
            ConstEdges getEdgesFromLocation(uint64_t index) const;
            
            /*!
             * Retrieves the edges of the location with the given index labeled with the given action index.
             */
            Edges getEdgesFromLocation(uint64_t locationIndex, uint64_t actionIndex);
            
            /*!
             * Retrieves the edges of the location with the given index.
             */
            ConstEdges getEdgesFromLocation(uint64_t locationIndex, uint64_t actionIndex) const;
            
            /*!
             * Adds an edge to the automaton.
             */
            void addEdge(Edge const& edge);
            
            /*!
             * Retrieves the edges of the automaton.
             */
            std::vector<Edge>& getEdges();

            /*!
             * Retrieves the edges of the automaton.
             */
            std::vector<Edge> const& getEdges() const;

            /*!
             * Retrieves the set of action indices that are labels of edges of this automaton.
             */
            std::set<uint64_t> getActionIndices() const;
            
            /*!
             * Retrieves the number of locations.
             */
            uint64_t getNumberOfLocations() const;

            /*!
             * Retrieves the number of edges.
             */
            uint64_t getNumberOfEdges() const;
            
            /*!
             * Retrieves whether the initial restriction is set and unequal to true
             */
            bool hasRestrictedInitialStates() const;
            
            /*!
             * Retrieves whether this automaton has an initial states restriction.
             */
            bool hasInitialStatesRestriction() const;
            
            /*!
             * Gets the expression restricting the legal initial values of the automaton's variables.
             */
            storm::expressions::Expression const& getInitialStatesRestriction() const;
            
            /*!
             * Sets the expression restricting the legal initial values of the automaton's variables.
             */
            void setInitialStatesRestriction(storm::expressions::Expression const& initialStatesRestriction);
            
            /*!
             * Retrieves the expression defining the legal initial values of the automaton's variables.
             */
            storm::expressions::Expression getInitialStatesExpression() const;
            
            /*!
             * Retrieves whether there is an edge labeled with the action with the given index in this automaton.
             */
            bool hasEdgeLabeledWithActionIndex(uint64_t actionIndex) const;
            
            /*!
             * Retrieves a list of expressions that characterize the legal values of the variables in this automaton.
             */
            std::vector<storm::expressions::Expression> getAllRangeExpressions() const;
            
            /*!
             * Substitutes all variables in all expressions according to the given substitution.
             */
            void substitute(std::map<storm::expressions::Variable, storm::expressions::Expression> const& substitution);
            
            /*!
             * Finalizes the building of this automaton. Subsequent changes to the automaton require another call to this
             * method. Note that this method is invoked by a call to <code>finalize</code> to the containing model.
             */
            void finalize(Model const& containingModel);
            
            /*!
             * Retrieves the action indices appearing at some edge of the automaton.
             */
            std::set<uint64_t> getUsedActionIndices() const;
            
            /*!
             * Checks whether the provided variables only appear in the probability expressions or the expressions being
             * assigned in transient assignments.
             */
            bool containsVariablesOnlyInProbabilitiesOrTransientAssignments(std::set<storm::expressions::Variable> const& variables) const;
            
            void pushEdgeAssignmentsToDestinations();
            
            /*!
             * Retrieves whether there is any transient edge destination assignment in the automaton.
             */
            bool hasTransientEdgeDestinationAssignments() const;
            
            /*!
             * Lifts the common edge destination assignments to edge assignments.
             */
            void liftTransientEdgeDestinationAssignments();
            
            /*!
             * Retrieves whether the automaton uses an assignment level other than zero.
             */
            bool usesAssignmentLevels() const;
            
        private:
            /// The name of the automaton.
            std::string name;

            /// The set of variables of this automaton.
            VariableSet variables;
            
            /// The locations of the automaton.
            std::vector<Location> locations;
            
            /// A mapping of location names to their indices.
            std::unordered_map<std::string, uint64_t> locationToIndex;
            
            /// All edges of the automaton
            std::vector<Edge> edges;
            
            /// A mapping from location indices to the starting indices. If l is mapped to i, it means that the edges
            /// leaving location l start at index i of the edges vector.
            std::vector<uint64_t> locationToStartingIndex;

            /// The indices of the initial locations.
            std::set<uint64_t> initialLocationIndices;
            
            /// The expression restricting the legal initial values of the variables of the automaton.
            storm::expressions::Expression initialStatesRestriction;
            
            /// The set of action indices that label some action in this automaton.
            std::set<uint64_t> actionIndices;
        };
        
    }
}
