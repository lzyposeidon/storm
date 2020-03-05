#include "storm/settings/modules/OviSolverSettings.h"

#include "storm/settings/Option.h"
#include "storm/settings/ArgumentBuilder.h"
#include "storm/settings/OptionBuilder.h"

#include "storm/utility/macros.h"
#include "storm/exceptions/IllegalArgumentValueException.h"

namespace storm {
    namespace settings {
        namespace modules {
            
            const std::string OviSolverSettings::moduleName = "ovi";
            const std::string OviSolverSettings::precisionUpdateFactorOptionName = "precision-update-factor";
            const std::string OviSolverSettings::maxVerificationIterationFactorOptionName = "max-verification-iter-factor";
            const std::string OviSolverSettings::useRelevantValuesForPrecisionUpdateOptionName = "use-relevant-values";
            const std::string OviSolverSettings::upperBoundGuessingFactorOptionName = "upper-bound-factor";
            const std::string OviSolverSettings::upperBoundOnlyIterationsOptionName = "check-upper-only-iter";
            
            OviSolverSettings::OviSolverSettings() : ModuleSettings(moduleName) {
                
                this->addOption(storm::settings::OptionBuilder(moduleName, precisionUpdateFactorOptionName, false, "Sets with which factor the precision of the inner value iteration is updated.").setIsAdvanced().addArgument(storm::settings::ArgumentBuilder::createDoubleArgument("factor", "The factor.").setDefaultValueDouble(0.4).addValidatorDouble(ArgumentValidatorFactory::createDoubleRangeValidatorExcluding(0.0, 1.0)).build()).build());
                
                this->addOption(storm::settings::OptionBuilder(moduleName, useRelevantValuesForPrecisionUpdateOptionName, false, "Sets whether the precision of the inner value iteration is only based on the relevant values (i.e. initial states).").setIsAdvanced().build());

                this->addOption(storm::settings::OptionBuilder(moduleName, maxVerificationIterationFactorOptionName, false, "Controls how many verification iterations are performed before guessing a new upper bound.").setIsAdvanced().addArgument(storm::settings::ArgumentBuilder::createDoubleArgument("factor", "The factor.").setDefaultValueDouble(0.1).addValidatorDouble(ArgumentValidatorFactory::createDoubleGreaterValidator(0.0)).build()).build());

                this->addOption(storm::settings::OptionBuilder(moduleName, upperBoundGuessingFactorOptionName, false, "Sets with which factor the precision is multiplied to guess the upper bound.").setIsAdvanced().addArgument(storm::settings::ArgumentBuilder::createDoubleArgument("factor", "The factor.").setDefaultValueDouble(1.0).addValidatorDouble(ArgumentValidatorFactory::createDoubleGreaterValidator(0.0)).build()).build());

                this->addOption(storm::settings::OptionBuilder(moduleName, upperBoundOnlyIterationsOptionName, false, "Sets the max. iterations OVI will only iterate over the upper bound.").setIsAdvanced().addArgument(storm::settings::ArgumentBuilder::createIntegerArgument("iter", "The iterations.").setDefaultValueInteger(20000).addValidatorInteger(ArgumentValidatorFactory::createIntegerGreaterValidator(0)).build()).build());
            }
            
            double OviSolverSettings::getPrecisionUpdateFactor() const {
                return this->getOption(precisionUpdateFactorOptionName).getArgumentByName("factor").getValueAsDouble();
            }
            
            double OviSolverSettings::getMaxVerificationIterationFactor() const {
                return this->getOption(maxVerificationIterationFactorOptionName).getArgumentByName("factor").getValueAsDouble();
            }
            
            bool OviSolverSettings::useRelevantValuesForPrecisionUpdate() const {
                return this->getOption(useRelevantValuesForPrecisionUpdateOptionName).getHasOptionBeenSet();
            }

            double OviSolverSettings::getUpperBoundGuessingFactor() const {
                return this->getOption(upperBoundGuessingFactorOptionName).getArgumentByName("factor").getValueAsDouble();
            }

            uint64_t OviSolverSettings::getUpperBoundOnlyIterations() const {
                return this->getOption(upperBoundOnlyIterationsOptionName).getArgumentByName("iter").getValueAsInteger();
            }
           
        }
    }
}