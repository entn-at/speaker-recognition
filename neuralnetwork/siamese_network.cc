/**
 * Train a multilayer perceptron to classify mnist digits
 *
 * This provide an example of usage of the mlp.h model
 */
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

    unsigned total_epoch = 1000;
    unsigned batch_size = 40;
    unsigned epoch_size = 2500;
    unsigned validation_size = 500;
    // ParameterCollection name (for saving) -----------------------------------------------------------------------

    ostringstream os;
    // Store a bunch of information in the model name
    os << "./models/syamese_network"
        << "_" << getpid()
        << ".params";
    const string fname = os.str();
    cerr << "Parameters will be written to: " << fname << endl;
    // Build model -----------------------------------------------------------------------------------

    ParameterCollection model;
    float learning_rate = 0.000001f;
    MomentumSGDTrainer trainer(model, learning_rate, 0.9);
    trainer.clip_threshold *= batch_size;

    // Create model
    MLP nn(model, vector<Layer>({
        Layer(/* input_dim */ 16896, /* output_dim */ 5000, /* activation */ RELU, /* dropout_rate */ 0.75),
        Layer(/* input_dim */ 5000, /* output_dim */ 5000, /* activation */ RELU, /* dropout_rate */ 0.75),
        Layer(/* input_dim */ 5000, /* output_dim */ 1000, /* activation */ RELU, /* dropout_rate */ 0.75),
        Layer(/* input_dim */ 1000, /* output_dim */ 5000, /* activation */ RELU, /* dropout_rate */ 0.75)
    }),vector<Layer>({
         Layer(/* input_dim */ 10000, /* output_dim */ 1000, /* activation */ RELU, /* dropout_rate */ 0.75),
         Layer(/* input_dim */ 1000, /* output_dim */ 1, /* activation */ SIGMOID, /* dropout_rate */ 0.0),
    }));

    // Initialize variables for training -------------------------------------------------------------
    double best_accuracy = 0;
    unsigned count_from_best_accuracy = 0;

    vector<Expression> cur_batch1, cur_batch2;
    vector<float> cur_labels;

    vector<vector<vector<float>>> train_data = loadData("train");
    vector<vector<vector<float>>> test_data = loadData("test");
    std::unique_ptr<Timer> iteration(new Timer("completed in"));
    for (unsigned epoch = 0; epoch < total_epoch; ++epoch) {

        // Start timer
        // Activate dropout
        nn.enable_dropout();
        float loss = 0;
        // Run for the given number of epochs
        for (unsigned i = 0; i < epoch_size; ++i) {
            // build graph for this instance
            ComputationGraph cg;
            // Get input batch
            cur_batch1 = vector<Expression>(batch_size);
            cur_batch2 = vector<Expression>(batch_size);
            cur_labels = vector<float>(batch_size);
            for (int j = 0; j < batch_size; j+=2) {
                Example ex = generateExample(train_data);
                cur_batch1[j] = input(cg, {16896}, *ex.positive1);
                cur_batch2[j] = input(cg, {16896}, *ex.positive2);
                cur_labels[j] = 1.0f;
                cur_batch1[j+1] = input(cg, {16896}, *ex.negative1);
                cur_batch2[j+1] = input(cg, {16896}, *ex.negative2);
                cur_labels[j+1] = 0.0f;
            }
            // Reshape as batch (not very intuitive yet)
            Expression x1_batch = reshape(concatenate_cols(cur_batch1), Dim({16896}, batch_size));
            Expression x2_batch = reshape(concatenate_cols(cur_batch2), Dim({16896}, batch_size));

            Expression labels_batch = reshape(input(cg, {batch_size}, cur_labels), Dim({1}, batch_size));
            Expression loss_expr = nn.get_nll(x1_batch, x2_batch, labels_batch, cg);

            loss += as_scalar(cg.forward(loss_expr));

            // Compute gradient with backward pass
            cg.backward(loss_expr);
            // Update parameters
            trainer.update();
            // Print progress
            cerr << "\r[TRAIN epoch="<< epoch <<"] Process: " << i*100/epoch_size << "% | " << " Loss = " << (loss / (i*batch_size));
        }
        cerr << endl;

        // Disable dropout for dev testing
        nn.disable_dropout();

        // Show score on dev data
        double hit_count = 0;
        for (unsigned i = 0; i < validation_size; ++i) {
            // build graph for this instance
            ComputationGraph cg;
            // Get input expression

            cur_batch1 = vector<Expression>(batch_size);
            cur_batch2 = vector<Expression>(batch_size);
            cur_labels = vector<float>(batch_size);
            for (int j = 0; j < batch_size; j+=2) {
                Example ex = generateExample(test_data);
                cur_batch1[j] = input(cg, {16896}, *ex.positive1);
                cur_batch2[j] = input(cg, {16896}, *ex.positive2);
                cur_labels[j] = 1.0f;
                cur_batch1[j+1] = input(cg, {16896}, *ex.negative1);
                cur_batch2[j+1] = input(cg, {16896}, *ex.negative2);
                cur_labels[j+1] = 0.0f;
            }
            // Reshape as batch (not very intuitive yet)
            Expression x1_batch = reshape(concatenate_cols(cur_batch1), Dim({16896}, batch_size));
            Expression x2_batch = reshape(concatenate_cols(cur_batch2), Dim({16896}, batch_size));

            vector<float> probs = nn.predict_batch(x1_batch,x2_batch,cg);

            for (int j = 0; j < batch_size; ++j) {
                if (int(probs[j] >= 0.5) == cur_labels[j])
                    ++hit_count;
            }

            cerr << "\r[DEV epoch="<< epoch << "] Process: " << i*100/validation_size << "%";
        }
        cerr << endl;
        // If the dev loss is lower than the previous ones, save the model
        if (hit_count > best_accuracy) {
            best_accuracy = hit_count;
            count_from_best_accuracy = 0;
            if (best_accuracy >= 0.80f) {
                TextFileSaver saver(fname);
                saver.save(model);
                cerr << "[SAVED]";
            }
            cerr << "[BEST epoch=" << epoch << " Accuracy: " << best_accuracy/ (double) (validation_size*batch_size) << "]" << endl;
        }
        else {
            ++count_from_best_accuracy;
            cerr << "[COUNT epoch=" << epoch << " count=" << count_from_best_accuracy << "]" << endl;
            if (count_from_best_accuracy >= 10) {
                cerr << "[BEST Accuracy: " << best_accuracy/ (double) (validation_size*batch_size) << "]" << endl;
                return 0;
            }
        }
        // Print informations
        cerr << "[DEV epoch=" << (epoch)
            << "] Accuracy = " << (hit_count / (double) (validation_size*batch_size))<< ' ';
        // Reinitialize timer
        iteration.reset(new Timer("completed in"));
        cerr << endl;
    }
}
