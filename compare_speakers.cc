#include "mlp.h"
#include "dynet/io.h"
#include "dynet/../examples/cpp-utils/getpid.h"
#include "dynet/../examples/cpp-utils/cl-args.h"
#include "dynet/../examples/cpp-utils/data-io.h"
#include "utils.h"

using namespace std;
using namespace dynet;

int main(int argc, char** argv) {
    // Fetch dynet params ----------------------------------------------------------------------------
    auto dyparams = dynet::extract_dynet_params(argc, argv);
    dynet::initialize(dyparams);
    // Fetch program specific parameters (see ../utils/cl-args.h) ------------------------------------
    Params params;

    get_args(argc, argv, params, TRAIN_SUP);

    // Build model -----------------------------------------------------------------------------------
    ParameterCollection model;

    // Create model
    MLP nn(model, vector<Layer>({
        Layer(/* input_dim */ 16896, /* output_dim */ 5000, /* activation */ RELU, /* dropout_rate */ 0.0),
        Layer(/* input_dim */ 5000, /* output_dim */ 5000, /* activation */ RELU, /* dropout_rate */ 0.0),
        Layer(/* input_dim */ 5000, /* output_dim */ 1000, /* activation */ RELU, /* dropout_rate */ 0.0),
        Layer(/* input_dim */ 1000, /* output_dim */ 5000, /* activation */ RELU, /* dropout_rate */ 0.0)
    }),vector<Layer>({
         Layer(/* input_dim */ 10000, /* output_dim */ 1000, /* activation */ RELU, /* dropout_rate */ 0.0),
         Layer(/* input_dim */ 1000, /* output_dim */ 1, /* activation */ SIGMOID, /* dropout_rate */ 0.0),
    }));


    //Load preexisting weights (if provided)
    if (params.model_file != "") {
        TextFileLoader loader(params.model_file);
        loader.populate(model);
    }

    ComputationGraph cg;
    // Get input expression

    Expression x1 = input(cg, {16896}, readSpeakerFile(params.dev_file));
    Expression x2 = input(cg, {16896}, readSpeakerFile(params.test_file));

    cout << nn.predict(x1, x2, cg) << endl;
}