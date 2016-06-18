#ifndef STAN_SERVICES_DIAGNOSE_DIAGNOSE_HPP
#define STAN_SERVICES_DIAGNOSE_DIAGNOSE_HPP

#include <stan/io/var_context.hpp>
#include <stan/io/chained_var_context.hpp>
#include <stan/io/random_var_context.hpp>
#include <stan/interface_callbacks/writer/base_writer.hpp>
#include <stan/model/util.hpp>
#include <boost/random/additive_combine.hpp>  // L'Ecuyer RNG
#include <vector>

namespace stan {
  namespace services {
    namespace diagnose {

      /**
       * Checks the gradients of the model computed using reverse mode
       * autodiff against finite differences.
       *
       * This will test the first order gradients using reverse mode
       * at the value specified in cont_params. This method only
       * outputs to the message_writer.
       *
       * @tparam Model A model implementation
       *
       * @param model Input model to test (with data already instantiated)
       * @param init var context for initialization
       * @param random_seed random seed for the pseudo random number generator
       * @param init_radius radius to initialize
       * @param chain chain id to advance the pseudo random number generator
       * @param epsilon epsilon to use for finite differences
       * @param error amount of absolute error to allow
       * @param message_writer Writer callback for display output
       * @param parameter_writer Writer callback for file output
       *
       * @return the number of parameters that are not within epsilon
       * of the finite difference calculation
       */
      template <class Model>
      int diagnose(Model& model,
                   stan::io::var_context& init,
                   unsigned int random_seed,
                   unsigned int chain,
                   double init_radius,
                   double epsilon,
                   double error,
                   interface_callbacks::writer::base_writer& message_writer,
                   interface_callbacks::writer::base_writer& parameter_writer) {
        boost::ecuyer1988 rng(random_seed);
        
        // Advance generator to avoid process conflicts
        static boost::uintmax_t DISCARD_STRIDE
          = static_cast<boost::uintmax_t>(1) << 50;
        rng.discard(DISCARD_STRIDE * (chain - 1));

        stan::io::random_var_context random_context(model, rng, init_radius);
        stan::io::chained_var_context context(init, random_context);
        
        std::vector<double> cont_vector;
        std::vector<int> disc_vector;
        std::stringstream ss;
        model.transform_inits(context,
                              disc_vector,
                              cont_vector,
                              &ss);
        message_writer(ss.str());

        message_writer("TEST GRADIENT MODE");

        int num_failed =
          stan::model::test_gradients<true, true>(model,
                                                  cont_vector, disc_vector,
                                                  epsilon, error,
                                                  message_writer);

        return num_failed;
      }

    }
  }
}
#endif
