#include <test/unit/mcmc/hmc/mock_hmc.hpp>
#include <stan/mcmc/hmc/base_hmc.hpp>

#include <boost/random/additive_combine.hpp>
#include <boost/algorithm/string/split.hpp>
#include <test/unit/util.hpp>
#include <gtest/gtest.h>

#include <stan/interface_callbacks/writer/stream_writer.hpp>

typedef boost::ecuyer1988 rng_t;

namespace stan {

  namespace mcmc {

    class mock_hmc: public base_hmc<mock_model,
                                    mock_hamiltonian,
                                    mock_integrator,
                                    rng_t> {

    public:

      mock_hmc(mock_model& model, rng_t& rng) :
        base_hmc<mock_model, mock_hamiltonian, mock_integrator, rng_t>
          (model, rng)
      { this->name_ = "Mock HMC"; }


      sample transition(sample& init_sample) {
        this->seed(init_sample.cont_params());
        return sample(this->z_.q, - this->hamiltonian_.V(this->z_), 0);
      }

      void get_sampler_param_names(std::vector<std::string>& names) {};
      void get_sampler_params(std::vector<double>& values) {};

    };

  }

}

TEST(McmcBaseHMC, point_construction) {

  rng_t base_rng(0);

  Eigen::VectorXd q(2);
  q(0) = 5;
  q(1) = 1;

  stan::mcmc::mock_model model(q.size());
  stan::mcmc::mock_hmc sampler(model, base_rng);

  EXPECT_EQ(q.size(), sampler.z().q.size());
  EXPECT_EQ(static_cast<int>(q.size()), sampler.z().g.size());

  EXPECT_EQ("", sampler.flush_info_buffer());
  EXPECT_EQ("", sampler.flush_err_buffer());
}

TEST(McmcBaseHMC, seed) {

  rng_t base_rng(0);

  Eigen::VectorXd q(2);
  q(0) = 5;
  q(1) = 1;

  stan::mcmc::mock_model model(q.size());
  stan::mcmc::mock_hmc sampler(model, base_rng);

  sampler.seed(q);

  for (int i = 0; i < q.size(); ++i)
    EXPECT_EQ(q(i), sampler.z().q(i));

  EXPECT_EQ("", sampler.flush_info_buffer());
  EXPECT_EQ("", sampler.flush_err_buffer());
}

TEST(McmcBaseHMC, set_nominal_stepsize) {
  rng_t base_rng(0);

  Eigen::VectorXd q(2);
  q(0) = 5;
  q(1) = 1;

  stan::mcmc::mock_model model(q.size());
  stan::mcmc::mock_hmc sampler(model, base_rng);

  double old_epsilon = 1.0;
  sampler.set_nominal_stepsize(old_epsilon);
  EXPECT_EQ(old_epsilon, sampler.get_nominal_stepsize());

  sampler.set_nominal_stepsize(-0.1);
  EXPECT_EQ(old_epsilon, sampler.get_nominal_stepsize());

  EXPECT_EQ("", sampler.flush_info_buffer());
  EXPECT_EQ("", sampler.flush_err_buffer());
}

TEST(McmcBaseHMC, set_stepsize_jitter) {
  rng_t base_rng(0);

  Eigen::VectorXd q(2);
  q(0) = 5;
  q(1) = 1;

  stan::mcmc::mock_model model(q.size());
  stan::mcmc::mock_hmc sampler(model, base_rng);

  double old_jitter = 0.1;
  sampler.set_stepsize_jitter(old_jitter);
  EXPECT_EQ(old_jitter, sampler.get_stepsize_jitter());

  sampler.set_nominal_stepsize(-0.1);
  EXPECT_EQ(old_jitter, sampler.get_stepsize_jitter());

  EXPECT_EQ("", sampler.flush_info_buffer());
  EXPECT_EQ("", sampler.flush_err_buffer());
}


TEST(McmcBaseHMC, streams) {
  stan::test::capture_std_streams();

  rng_t base_rng(0);

  Eigen::VectorXd q(2);
  q(0) = 5;
  q(1) = 1;

  stan::mcmc::mock_model model(q.size());

  EXPECT_NO_THROW(stan::mcmc::mock_hmc sampler(model, base_rng));

  stan::mcmc::mock_hmc sampler(model, base_rng);

  std::stringstream writer_ss;
  stan::interface_callbacks::writer::stream_writer writer(writer_ss);
  EXPECT_NO_THROW(sampler.write_sampler_state(writer));
  EXPECT_EQ("# Step size = 0.1\n",
            writer_ss.str());

  stan::test::reset_std_streams();
  EXPECT_EQ("", stan::test::cout_ss.str());
  EXPECT_EQ("", stan::test::cerr_ss.str());
}
